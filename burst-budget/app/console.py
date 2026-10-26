#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Local, receive-only Burst Budget console. No third-party dependencies."""
import argparse
from collections import deque
import json
import os
from pathlib import Path
import re
import select
import stat
import termios
import time
import tkinter as tk
from tkinter import ttk, filedialog, messagebox

FIELDS = ('now phase levels qualified tokens refill empty_age faults admissions '
          'allowed excess denied missing gaps segments tx_fail minted discarded').split()
COUNTERS = FIELDS[8:]
PHASES = ('UNKNOWN', 'TRACKING', 'HALTED')
FAULTS = ('excess admission', 'denied admission', 'missing throttle', 'sample gap',
          'transmit loss', 'counter exhausted', 'watchdog startup failure')
LIMIT = 2*1024*1024

def crc(data):
    result = 0xffff
    for byte in data:
        result ^= byte << 8
        for _ in range(8):
            result = ((result << 1) ^ (0x1021 if result & 0x8000 else 0)) & 0xffff
    return result

def validate(s):
    if s['phase'] > 2 or s['levels'] > 15 or s['qualified'] > 15:
        raise ValueError('state range')
    if s['tokens'] > 4 or s['refill'] >= 1000 or s['empty_age'] > 20 or s['faults'] > 127:
        raise ValueError('policy range')
    if s['allowed'] + s['excess'] + s['denied'] != s['admissions']:
        raise ValueError('admission conservation')
    if s['allowed'] + s['discarded'] + s['tokens'] != s['minted']:
        raise ValueError('credit conservation')
    for key, mask in [('excess',1),('denied',2),('missing',4),('gaps',8),('tx_fail',16)]:
        if s[key] and not s['faults'] & mask:
            raise ValueError('missing sticky finding')
    if bool(s['faults'] & 96) != (s['phase'] == 2):
        raise ValueError('terminal state')
    if s['phase'] == 0 and (s['tokens'] or s['refill'] or s['empty_age']):
        raise ValueError('unknown credit')
    if s['phase'] == 1 and (s['qualified'] != 15 or not s['levels'] & 4):
        raise ValueError('tracking without reset release')
    if s['tokens'] and s['empty_age'] or s['tokens'] == 4 and s['refill']:
        raise ValueError('bucket state')

class Model:
    def __init__(self, source='replay'):
        self.source = source
        self.connected = False
        self.buffer = bytearray()
        self.status = None
        self.history = deque(maxlen=256)
        self.errors = 0
        self.policy = False
        self.frozen = ''
        self.last_progress = None
        self.anchor_host = None
        self.elapsed = 0
        self.boot = None

    def fresh(self, now=None):
        now = time.monotonic() if now is None else now
        return bool(self.source == 'live' and self.connected and self.policy and
                    not self.frozen and self.last_progress is not None and
                    0 <= now-self.last_progress <= 1)

    def feed(self, data, now=None):
        now = time.monotonic() if now is None else now
        if self.source == 'live' and self.last_progress is not None and now-self.last_progress > 1:
            self.frozen = self.frozen or 'Reception pause: reconnect and flush before a new trial'
        # Process incrementally so even a hostile large chunk stays bounded.
        for byte in data:
            self.buffer.append(byte)
            if self.buffer.endswith(b'BB1,') and len(self.buffer) > 4:
                self.errors += 1
                self.buffer[:] = b'BB1,'
            if byte == 10:
                frame = bytes(self.buffer[:-1]); self.buffer.clear()
                try:
                    self.accept(frame, now)
                except ValueError:
                    self.errors += 1
            elif len(self.buffer) >= 384:
                self.errors += 1
                self.buffer.clear()

    def accept(self, frame, now):
        match = re.fullmatch(rb'(BB1,[BPS](?:,[0-9]{1,10})+)\*([0-9A-F]{4})', frame)
        if not match or crc(match[1]) != int(match[2],16):
            raise ValueError('framing or CRC')
        fields = match[1].decode('ascii').split(',')
        kind, values = fields[1], list(map(int, fields[2:]))
        if any(v > 0xffffffff for v in values):
            raise ValueError('integer range')
        if kind == 'P':
            if values != [4,1000,20,3,20]:
                self.frozen = 'Incompatible policy'; raise ValueError(self.frozen)
            self.policy = True
            return
        if kind == 'B':
            if len(values) != 1:
                raise ValueError('boot length')
            if self.status is not None:
                self.frozen = 'Observed reboot: start a new session'
            self.boot = values[0]
            return
        if len(values) != len(FIELDS):
            raise ValueError('status length')
        s = dict(zip(FIELDS,values))
        validate(s)
        if self.frozen:
            return
        if self.status is not None:
            old = self.status
            delta = (s['now']-old['now']) & 0xffffffff
            if delta == 0:
                if s != old:
                    self.frozen = 'Contradictory duplicate'
                return
            if delta >= 0x80000000 or any(s[k]<old[k] for k in COUNTERS) or old['faults'] & ~s['faults']:
                self.frozen = 'Counter, clock or sticky finding rollback'
                return
            if old['phase'] == 2 and any(s[k] != old[k] for k in FIELDS if k not in ('now','tx_fail','faults')):
                self.frozen = 'Acquisition changed after terminal halt'
                return
            if s['admissions']-old['admissions'] > delta or s['missing']-old['missing'] > delta:
                raise ValueError('counter progress exceeds sampled time')
            # Generous necessary bound; not proof of exact waveform realizability.
            if s['minted']-old['minted'] > (old['refill']+delta)//1000:
                raise ValueError('impossible refill rate')
            self.elapsed += delta
        else:
            self.anchor_host = now
        if self.source == 'live' and abs(now-self.anchor_host-self.elapsed/1000) > 0.5+(now-self.anchor_host)*0.05:
            self.frozen = 'Host/device clock divergence: start a new session'
            return
        self.status = s
        self.last_progress = now
        self.history.append(s.copy())

    def finish(self):
        if self.buffer:
            self.errors += 1; self.buffer.clear()
        if self.status is None:
            raise ValueError('No valid status in recording')

    def export(self, path):
        now = time.monotonic()
        report = dict(author='jayis1', protocol='BB1', source=self.source,
                      connected=self.connected, fresh=self.fresh(now), errors=self.errors,
                      policy=self.policy, warning=self.frozen, boot=self.boot,
                      receipt_age=None if self.last_progress is None else now-self.last_progress,
                      status=self.status, history=list(self.history))
        fd = os.open(path, os.O_WRONLY|os.O_CREAT|os.O_EXCL, 0o600)
        with os.fdopen(fd,'w') as output:
            json.dump(report,output,indent=2); output.write('\n')

def read_replay(path):
    fd = os.open(path,os.O_RDONLY|os.O_NONBLOCK)
    with os.fdopen(fd,'rb') as stream:
        info = os.fstat(stream.fileno())
        if not stat.S_ISREG(info.st_mode) or info.st_size > LIMIT:
            raise ValueError('Replay must be a regular file of at most 2 MiB')
        data = stream.read(LIMIT+1)
        if len(data)>LIMIT:
            raise ValueError('Replay grew beyond limit')
        return data

class Serial:
    def __init__(self, path):
        self.fd = os.open(path,os.O_RDONLY|os.O_NOCTTY|os.O_NONBLOCK)
        self.saved = None
        try:
            self.saved = termios.tcgetattr(self.fd)
            attrs = termios.tcgetattr(self.fd)
            attrs[0]=0; attrs[1]=0; attrs[3]=0
            attrs[2]=termios.CS8|termios.CREAD|termios.CLOCAL
            attrs[4]=termios.B115200; attrs[5]=termios.B115200
            attrs[6][termios.VMIN]=0; attrs[6][termios.VTIME]=0
            termios.tcsetattr(self.fd,termios.TCSANOW,attrs)
            termios.tcflush(self.fd,termios.TCIFLUSH)
            self.poller=select.poll()
            self.poller.register(self.fd,select.POLLIN|select.POLLERR|select.POLLHUP)
        except BaseException:
            self.close(); raise

    def read(self):
        if self.fd is None:
            return b'',True
        events=self.poller.poll(0)
        if not events:
            return b'',False
        mask=events[0][1]
        data=b''
        if mask & select.POLLIN:
            try: data=os.read(self.fd,8192)
            except BlockingIOError: pass
        return data,bool(mask & (select.POLLERR|select.POLLHUP|select.POLLNVAL))

    def close(self):
        if self.fd is None:
            return
        try:
            if self.saved is not None:
                termios.tcsetattr(self.fd,termios.TCSANOW,self.saved)
        except (OSError,termios.error):
            pass
        finally:
            os.close(self.fd); self.fd=None

class Console:
    def __init__(self, root):
        self.root=root; self.model=Model(); self.serial=None
        root.title('Burst Budget — jayis1')
        self.port=tk.StringVar(value='/dev/ttyACM0')
        bar=ttk.Frame(root); bar.pack(fill='x')
        ttk.Entry(bar,textvariable=self.port).pack(side='left')
        for label,command in [('New live session',self.connect),('Disconnect',self.disconnect),
                              ('Replay',self.replay),('Export',self.export)]:
            ttk.Button(bar,text=label,command=command).pack(side='left')
        self.book=ttk.Notebook(root); self.book.pack(fill='both',expand=True)
        self.views=[]
        for title in ('Budget','History','Integrity'):
            view=tk.Text(self.book,width=100,height=24,wrap='word')
            self.book.add(view,text=title); self.views.append(view)
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.timer=root.after(50,self.poll)
        self.render()

    def connect(self):
        self.disconnect()
        try:
            port=Serial(self.port.get())
        except (OSError,termios.error) as exc:
            messagebox.showerror('Connection failed',str(exc)); return
        self.model=Model('live'); self.serial=port; self.model.connected=True
        self.render()

    def disconnect(self):
        if self.serial is not None:
            self.serial.close(); self.serial=None
        self.model.connected=False

    def load(self,path):
        data=read_replay(path)
        model=Model('replay'); model.feed(data); model.finish()
        self.disconnect(); self.model=model; self.render()

    def replay(self):
        path=filedialog.askopenfilename()
        if path:
            try: self.load(path)
            except (OSError,ValueError) as exc: messagebox.showerror('Replay failed',str(exc))

    def export(self):
        path=filedialog.asksaveasfilename(defaultextension='.json')
        if path:
            try: self.model.export(path)
            except OSError as exc: messagebox.showerror('Export failed',str(exc))

    def poll(self):
        if self.serial is not None:
            try:
                data,hangup=self.serial.read()
                self.model.feed(data)
                if hangup: self.disconnect()
            except (OSError,termios.error):
                self.disconnect()
        self.render()
        self.timer=self.root.after(50,self.poll)

    def render(self):
        m=self.model; s=m.status
        budget='No accepted status. Check connection and policy.\n'
        if s is not None:
            findings=', '.join(name for i,name in enumerate(FAULTS) if s['faults']&(1<<i)) or 'none observed'
            budget=(f"Phase: {PHASES[s['phase']]}\nCredits: {s['tokens']} / 4\n"
                    f"Refill progress: {s['refill']} / 1000 sampled ms\n"
                    f"Admissions: {s['admissions']}   Allowed: {s['allowed']}\n"
                    f"Excess: {s['excess']}   Denied: {s['denied']}\n"
                    f"Missing throttle samples: {s['missing']}\nFindings: {findings}\n"
                    'Observation only — not an enforcement or safety device.\n')
        integrity=(f'Source: {m.source}\nConnected: {m.connected}\nFresh: {m.fresh()}\n'
                   f'Compatible policy: {m.policy}\nParser errors: {m.errors}\n'
                   f'Warning: {m.frozen or "none"}\nBoot flags: {m.boot}\n'
                   'CRC is not authentication. Reboots may be indistinguishable.\n'
                   'History is bounded to 256 snapshots. Export before replacing a session.\n')
        history='time  credits  allowed  excess  denied  faults\n'+'\n'.join(
            '  '.join(str(row[k]) for k in ('now','tokens','allowed','excess','denied','faults')) for row in m.history)
        for view,text in zip(self.views,(budget,history,integrity)):
            view.configure(state='normal'); view.delete('1.0','end'); view.insert('end',text)
            view.configure(state='disabled')

    def close(self):
        self.root.after_cancel(self.timer); self.disconnect(); self.root.destroy()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--replay',type=Path)
    parser.add_argument('--export',type=Path)
    args=parser.parse_args()
    if args.replay:
        m=Model(); m.feed(read_replay(args.replay)); m.finish()
        if args.export: m.export(args.export)
        print(json.dumps({'author':'jayis1','status':m.status,'errors':m.errors,'warning':m.frozen}))
        return 1 if m.errors or m.frozen or not m.policy else 0
    root=tk.Tk(); Console(root); root.mainloop()
    return 0
if __name__=='__main__':
    raise SystemExit(main())
