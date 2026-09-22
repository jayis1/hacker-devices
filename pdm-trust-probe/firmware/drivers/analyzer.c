/* PDM anomaly classifier; stores statistics, never audio payloads
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#include "analyzer.h"
#include <string.h>
static void emit(ptp_analyzer_t *a,uint8_t type,uint8_t ch,uint16_t detail,uint32_t value,uint64_t ts){
 ptp_event_t *e; if(a->count==PTP_EVENT_CAPACITY){ a->read_index=(a->read_index+1u)%PTP_EVENT_CAPACITY; a->count--; a->event_drops++; }
 e=&a->events[a->write_index]; e->type=type;e->channel=ch;e->detail=detail;e->value=value;e->timestamp=ts;
 a->write_index=(a->write_index+1u)%PTP_EVENT_CAPACITY;a->count++;a->violations++;
}
void analyzer_init(ptp_analyzer_t *a){ if(!a)return; memset(a,0,sizeof(*a)); a->policy.density_min_permille=120u;a->policy.density_max_permille=880u;a->policy.transition_min_permille=40u;a->policy.transition_max_permille=900u;a->policy.clock_min_hz=1000000u;a->policy.clock_max_hz=3500000u;a->policy.enabled_channels=0x0Fu; }
bool analyzer_set_policy(ptp_analyzer_t *a,const ptp_policy_t *p){ if(!a||!p||p->density_min_permille>=p->density_max_permille||p->transition_min_permille>=p->transition_max_permille||p->clock_min_hz<PTP_CLOCK_MIN_HZ||p->clock_max_hz>PTP_CLOCK_MAX_HZ||p->clock_min_hz>=p->clock_max_hz||(p->enabled_channels&0x0Fu)==0u)return false; a->policy=*p; return true; }
void analyzer_consume(ptp_analyzer_t *a,const ptp_window_t *w,uint32_t clock_hz){ unsigned ch; if(!a||!w||w->clocks==0u)return; a->windows_seen++;
 if(clock_hz<a->policy.clock_min_hz||clock_hz>a->policy.clock_max_hz) emit(a,EVT_CLOCK,0xFFu,0u,clock_hz,w->timestamp);
 for(ch=0u;ch<PTP_CHANNEL_COUNT;ch++){
  uint32_t density,transition; if((w->channel_mask&(1u<<ch))==0u||(a->policy.enabled_channels&(1u<<ch))==0u)continue;
  density=(w->ones[ch]*1000u)/w->clocks; transition=(w->transitions[ch]*1000u)/w->clocks;
  if(w->ones[ch]==0u||w->ones[ch]==w->clocks) emit(a,EVT_STUCK,(uint8_t)ch,0u,w->ones[ch],w->timestamp);
  else if(density<a->policy.density_min_permille||density>a->policy.density_max_permille) emit(a,EVT_DENSITY,(uint8_t)ch,(uint16_t)density,w->ones[ch],w->timestamp);
  if(transition<a->policy.transition_min_permille||transition>a->policy.transition_max_permille) emit(a,EVT_TRANSITION,(uint8_t)ch,(uint16_t)transition,w->transitions[ch],w->timestamp);
  if(ch>0u&&(w->flags&(1u<<ch))!=0u) emit(a,EVT_DUPLICATE,(uint8_t)ch,0u,w->ones[ch],w->timestamp);
 }
}
bool analyzer_next_event(ptp_analyzer_t *a,ptp_event_t *e){ if(!a||!e||a->count==0u)return false;*e=a->events[a->read_index];a->read_index=(a->read_index+1u)%PTP_EVENT_CAPACITY;a->count--;return true; }
