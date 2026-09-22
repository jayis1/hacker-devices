/* PDM capture and bounded-window statistics
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#include "pdm_capture.h"
#include <string.h>
static unsigned popcount8(uint8_t value){ unsigned n=0u; while(value){ n+=(unsigned)(value&1u); value>>=1u;} return n; }
void capture_init(ptp_capture_t *capture){ if(capture){ memset(capture,0,sizeof(*capture)); capture->clock_hz=3072000u; } }
bool capture_configure(ptp_capture_t *capture,uint32_t clock_hz){ if(!capture||capture->running||clock_hz<PTP_CLOCK_MIN_HZ||clock_hz>PTP_CLOCK_MAX_HZ) return false; capture->clock_hz=clock_hz; return true; }
void capture_start(ptp_capture_t *capture){ if(capture) capture->running=true; }
void capture_stop(ptp_capture_t *capture){ if(capture) capture->running=false; }
bool capture_push_bits(ptp_capture_t *capture,const uint8_t *data,size_t clocks,uint8_t mask,uint64_t timestamp){
 ptp_window_t *w; uint8_t previous[PTP_CHANNEL_COUNT]={0}; bool seen[PTP_CHANNEL_COUNT]={0}; size_t i; unsigned ch;
 if(!capture||!data||!capture->running||clocks==0u||clocks>4096u||(mask&0x0Fu)==0u) return false;
 if(capture->count==64u){ capture->read_index=(capture->read_index+1u)%64u; capture->count--; capture->dropped++; }
 w=&capture->windows[capture->write_index]; memset(w,0,sizeof(*w)); w->timestamp=timestamp; w->clocks=(uint32_t)clocks; w->channel_mask=(uint8_t)(mask&0x0Fu);
 for(ch=0u;ch<PTP_CHANNEL_COUNT;ch++) w->fingerprints[ch]=2166136261u;
 for(i=0u;i<clocks;i++){
  uint8_t sample=data[i];
  for(ch=0u;ch<PTP_CHANNEL_COUNT;ch++){
   if((mask&(1u<<ch))!=0u){ uint8_t bit=(uint8_t)((sample>>ch)&1u); w->ones[ch]+=bit; if(seen[ch]&&bit!=previous[ch]) w->transitions[ch]++; previous[ch]=bit; seen[ch]=true; w->fingerprints[ch]^=bit; w->fingerprints[ch]*=16777619u; }
  }
 }
 if(popcount8(mask)>1u){ for(ch=1u;ch<PTP_CHANNEL_COUNT;ch++) if((mask&(1u<<ch))&&w->ones[ch]==w->ones[0]&&w->fingerprints[ch]==w->fingerprints[0]) w->flags|=(uint8_t)(1u<<ch); }
 capture->write_index=(capture->write_index+1u)%64u; capture->count++; return true;
}
bool capture_next(ptp_capture_t *capture,ptp_window_t *window){ if(!capture||!window||capture->count==0u) return false; *window=capture->windows[capture->read_index]; capture->read_index=(capture->read_index+1u)%64u; capture->count--; return true; }
