#ifndef __SDO_DATA_H__
#define __SDO_DATA_H__

typedef enum SDO_IMAGE_TYPE {
    AIA_94 = 0,
    AIA_131,
    AIA_171,
    AIA_193,
    AIA_211,
    AIA_304,
    AIA_335,
    AIA_1600,
    AIA_1700,
    AIA_4500,
    HMI_IC,
    HMI_IC_FLAT,
    HMI_M,
    MHI_M_COLOR,
    SDO_IMAGE_TYPE_MAX
} sdo_image_type;


#define SUCCESS 1
#define FAILED  0

#endif //__SDO_DATA_H__


