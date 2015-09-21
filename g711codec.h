/*
 * G711 encode decode HEADER.
 */

#ifndef	__G711CODEC_H__
#define	__G711CODEC_H__

int PCM2G711a( char *InAudioData, char *OutAudioData, int DataLen, int reserve );
int PCM2G711u( char *InAudioData, char *OutAudioData, int DataLen, int reserve );

int G711a2PCM( char *InAudioData, char *OutAudioData, int DataLen, int reserve );
int G711u2PCM( char *InAudioData, char *OutAudioData, int DataLen, int reserve );

int g711a_decode(short amp[], const unsigned char g711a_data[], int g711a_bytes);

int g711u_decode(short amp[], const unsigned char g711u_data[], int g711u_bytes);

int g711a_encode(unsigned char g711_data[], const short amp[], int len);

int g711u_encode(unsigned char g711_data[], const short amp[], int len);

#endif  /* g711codec.h */
