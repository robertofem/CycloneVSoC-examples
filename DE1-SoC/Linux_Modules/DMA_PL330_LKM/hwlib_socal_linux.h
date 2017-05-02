//-------define the chip used. Needed for some options in hwlib library-------//
#ifndef soc_cv_av
  #define soc_cv_av
#endif

//----------------------code from hwlib.h--------------------//
typedef int32_t             ALT_STATUS_CODE;

/*! Definitions of status codes returned by the HWLIB. */

/*! The operation was successful. */
#define ALT_E_SUCCESS               0

/*! The operation failed. */
#define ALT_E_ERROR                 (-1)
/*! FPGA configuration error detected.*/
#define ALT_E_FPGA_CFG              (-2)
/*! FPGA CRC error detected. */
#define ALT_E_FPGA_CRC              (-3)
/*! An error occurred on the FPGA configuration bitstream input source. */
#define ALT_E_FPGA_CFG_STM          (-4)
/*! The FPGA is powered off. */
#define ALT_E_FPGA_PWR_OFF          (-5)
/*! The SoC does not currently control the FPGA. */
#define ALT_E_FPGA_NO_SOC_CTRL      (-6)
/*! The FPGA is not in USER mode. */
#define ALT_E_FPGA_NOT_USER_MODE    (-7)
/*! An argument violates a range constraint. */
#define ALT_E_ARG_RANGE             (-8)
/*! A bad argument value was passed. */
#define ALT_E_BAD_ARG               (-9)
/*! The operation is invalid or illegal. */
#define ALT_E_BAD_OPERATION         (-10)
/*! An invalid option was selected. */
#define ALT_E_INV_OPTION            (-11)
/*! An operation or response timeout period expired. */
#define ALT_E_TMO                   (-12)
/*! The argument value is reserved or unavailable. */
#define ALT_E_RESERVED              (-13)
/*! A clock is not enabled or violates an operational constraint. */
#define ALT_E_BAD_CLK               (-14)
/*! The version ID is invalid. */
#define ALT_E_BAD_VERSION           (-15)
/*! The buffer does not contain enough free space for the operation. */
#define ALT_E_BUF_OVF               (-20)