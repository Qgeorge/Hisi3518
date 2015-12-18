/* ------------------------------------------------------------------
 * Copyright (C) 2009 Martin Storsjo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include "interf_enc.h"
int count_amr = 1;
int PCM2AMR(char *pStream, int len, char *outbuf)
{
	
	void *amr;
	int dtx = 0;
	enum Mode mode = MR122;
	short buf[160 * 2];
	int ret = 0, i = 0;
	amr = Encoder_Interface_init(dtx);
	for (i = 0; i < 160 * 2; i++) {
		const uint8_t* in = (pStream + 2*i);
		buf[i] = in[0] | (in[1] << 8);
	}
	ret = Encoder_Interface_Encode(amr, mode, buf, outbuf + 32*(count_amr - 1), 0);
	ret = Encoder_Interface_Encode(amr, mode, buf + 160, outbuf + 32*count_amr, 0);
	count_amr = count_amr + 2;
	if(count_amr >= 8)
	{
		count_amr = 1;
	}
	Encoder_Interface_exit(amr);
	return ret * 2;
}
