/**
 * @file    Kensa.h
 *
 * @date    2019/06/24
 * @author  haya
 * @note	•¶ŽšƒR[ƒh•ÏŠ· SJIS->UTF8
 */

#ifndef _KENSA_H_
#define _KENSA_H_


#ifdef EDF
#undef EDF
#endif

#ifdef _KENSA_C_
#define EDF
#else
#define EDF extern
#endif

EDF uint16_t KensaMain( int16_t cmd, char *p_in, uint16_t sz_in, char **p_out, uint16_t *sz_out );

/* static‚Ö
EDF uint16_t KsSerial(void);
EDF uint16_t KsFactory(void);
EDF uint16_t KsSRAM(void);
EDF uint16_t KsMac(void);
*/

#endif /* _KENSA_H_ */
