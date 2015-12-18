#ifndef __HI_OV7725_H__
#define __HI_OV7725_H__

/*SET CMD*/
#define DC_SET_IMAGESIZE       0x01    /* imagesize£¬value_range£º 
                                        DC_VAL_VGA
                                        DC_VAL_QVGA
                                        DC_VAL_QQVGA 
                                       */

#define DC_SET_BRIGHT          0x02    /* bright£¬value_range:0~255 */
#define DC_SET_CONTRACT        0x03    /* contrast£¬value_range£º0~255 */
#define DC_SET_HUE             0x04    /* hue£¬value_range£º-128~127 */
#define DC_SET_SATURATION      0x05    /* saturation£¬value_range£º0~255 */
#define DC_SET_SHARPNESS       0x06    /* sharpness£¬value_range£º0~16 */

#define DC_SET_ADNSWITCH       0x07    /* automatic de_noise switch£¬value_range£º
                                        DC_VAL_ON
                                        DC_VAL_OFF
                                       */
#define DC_SET_DNT             0x08    /* de_noise switch value£¬value_range£º0~255 */


#define DC_SET_AWBSWITCH       0x09    /* automatic white blance switch£¬value_range£º
                                        DC_VAL_ON
                                        DC_VAL_OFF
                                       */
#define DC_SET_WBR             0x0A    /* R_value, value_range£º0~255 */
#define DC_SET_WBB             0x0B    /* B_value, value_range£º0~255 */

#define DC_SET_AECSWITCH       0x0C    /* automatic exposure control,value_range£º
                                        DC_VAL_ON
                                        DC_VAL_OFF
                                       */
#define DC_SET_EC              0x0D    /* manual exposure control£¬value_range£º0~255 */

#define DC_SET_AGCSWITCH       0x0E    /* automatic gain control switch£¬value_range£º
                                        DC_VAL_ON
                                        DC_VAL_OFF
                                       */
#define DC_SET_GC              0x0F    /* manual gain value£¬value_range£º0~255 */

#define DC_SET_ABLCSWITCH      0x10    /* automatic black_level calibration£¬value_range£º
                                        DC_VAL_ON
                                        DC_VAL_OFF
                                       */

#define DC_SET_COLOR           0x11    /* color£¬value_range£º
                                        DC_VAL_COLOR
                                        DC_VAL_BAW      (black and white) 
                                       */

#define DC_SET_SCENE           0x12    /* scene£¬value_range£º
                                        DC_VAL_OUTDOOR
                                        DC_VAL_INDOOR
                                        DC_VAL_MANUAL
                                       */

#define DC_SET_POWERFREQ       0x13    /* power frequency£¬value_range£º
                                        DC_VAL_50HZ
                                        DC_VAL_60HZ
                                       */

#define DC_SET_FLIP            0x14    /* flip£¬value_range£º
                                        DC_VAL_ON
                                        DC_VAL_OFF
                                       */

#define DC_SET_MIRROR          0x15    /* mirror£¬value_range£º
                                        DC_VAL_ON
                                        DC_VAL_OFF
                                       */

/*===============================================================*/
/*GET CMD*/
#define DC_GET_IMAGESIZE       0x21    /* imagesize£¬value_range£º 
                                        DC_VAL_VGA
                                        DC_VAL_QVGA
                                        DC_VAL_QQVGA 
                                       */

#define DC_GET_BRIGHT          0x22    /* bright£¬value_range:0~255 */
#define DC_GET_CONTRACT        0x23    /* contrast£¬value_range£º0~255 */
#define DC_GET_HUE             0x24    /* hue£¬value_range£º-128~127 */
#define DC_GET_SATURATION      0x25    /* saturation£¬value_range£º0~255 */
#define DC_GET_SHARPNESS       0x26    /* sharpness£¬value_range£º0~255 */

#define DC_GET_ADNSWITCH       0x27    /* automatic de_noise switch£¬value_range£º
                                        DC_VAL_ON
                                        DC_VAL_OFF
                                       */
#define DC_GET_DNT             0x28    /* de_noise switch value£¬value_range£º0~255 */


#define DC_GET_AWBSWITCH       0x29    /* automatic white blance switch£¬value_range£º
                                        DC_VAL_ON
                                        DC_VAL_OFF
                                       */
#define DC_GET_WBR             0x2A    /* R_value value_range£º0~255 */
#define DC_GET_WBB             0x2B    /* B_value value_range£º0~255 */

#define DC_GET_AECSWITCH       0x2C    /* automatic exposure control,value_range£º
                                        DC_VAL_ON
                                        DC_VAL_OFF
                                       */
#define DC_GET_EC              0x2D    /* manual exposure control£¬value_range£º0~255 */

#define DC_GET_AGCSWITCH       0x2E    /*automatic gain control switch£¬value_range£º
                                        DC_VAL_ON
                                        DC_VAL_OFF
                                       */
#define DC_GET_GC              0x2F    /* manual gain value£¬value_range£º0~255 */

#define DC_GET_ABLCSWITCH      0x30    /* automatic black_level calibration£¬value_range£º
                                        DC_VAL_ON
                                        DC_VAL_OFF
                                       */

#define DC_GET_COLOR           0x31    /* color£¬value_range£º
                                        DC_VAL_COLOR
                                        DC_VAL_BAW      (black and white) 
                                       */

#define DC_GET_SCENE           0x32    /* scene£¬value_range£º
                                        DC_VAL_OUTDOOR  
                                        DC_VAL_INDOOR   
                                        DC_VAL_MANUAL   
                                       */

#define DC_GET_POWERFREQ       0x33    /* power frequency£¬value_range£º
                                        DC_VAL_50HZ
                                        DC_VAL_60HZ
                                       */

#define DC_GET_FLIP            0x34    /* flip£¬value_range£º
                                        DC_VAL_ON
                                        DC_VAL_OFF
                                       */

#define DC_GET_MIRROR          0x35    /* mirror£¬value_range£º
                                        DC_VAL_ON
                                        DC_VAL_OFF
                                       */


/*===============================================================*/
/*Value Define*/

#define    DC_VAL_VGA          0x01    /* imagesize£¬VGA value*/
#define    DC_VAL_QVGA         0x02    /* imagesize£¬QVGA value*/
#define    DC_VAL_QQVGA        0x03    /* imagesize£¬QQVGA value*/

#define    DC_VAL_ON           0x01
#define    DC_VAL_OFF          0x02

#define    DC_VAL_COLOR        0x01    /*color£¬color*/
#define    DC_VAL_BAW          0x02    /*color£¬black and white */

#define    DC_VAL_OUTDOOR      0x01    /*scene£¬outdoor*/
#define    DC_VAL_INDOOR       0x02    /*scene£¬indoor*/
#define    DC_VAL_MANUAL       0x03    /*scene£¬manual*/

#define    DC_VAL_50HZ         0x01    /*power frequency,50HZ*/
#define    DC_VAL_60HZ         0x02    /*power frequency,60HZ*/

#endif /*__HI_OV7725_H__*/

