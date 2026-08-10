/* worker/src/discovery.c — locate the master for a worker.
 *
 * TODO:
 * - Implement configured endpoint fallback.
 * - Add optional multicast or mDNS discovery behind an explicit option.
 * - Cache the last valid endpoint and notify worker transport of changes.
 * - Keep discovery independent from chunk execution.
 */

#include "worker/discovery.h"
