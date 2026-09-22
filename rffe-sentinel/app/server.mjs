// RFFE Sentinel local static server
// Author: jayis1
// SPDX-License-Identifier: MIT
import { createServer } from 'node:http';
import { readFile } from 'node:fs/promises';
import { extname, join } from 'node:path';

const port = Number(process.env.PORT || 4173);
const root = new URL('.', import.meta.url).pathname;
const types = { '.html': 'text/html', '.js': 'text/javascript', '.css': 'text/css' };

createServer(async (request, response) => {
  const relative = request.url === '/' ? 'index.html' : request.url.slice(1);
  if (!['index.html', 'app.js', 'styles.css'].includes(relative)) {
    response.writeHead(404).end('Not found');
    return;
  }
  try {
    const body = await readFile(join(root, relative));
    response.writeHead(200, { 'Content-Type': types[extname(relative)], 'Cache-Control': 'no-store' });
    response.end(body);
  } catch {
    response.writeHead(500).end('Unable to read application file');
  }
}).listen(port, '127.0.0.1', () => {
  console.log(`RFFE Sentinel app by jayis1: http://127.0.0.1:${port}`);
});
