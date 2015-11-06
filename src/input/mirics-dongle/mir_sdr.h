//
// Copyright (c) 2013 Mirics Ltd, All Rights Reserved
//

#ifndef MIR_SDR_H
#define MIR_SDR_H

#ifndef _MIR_SDR_QUALIFIER
#if !defined(STATIC_LIB) && (defined(_M_X64) || defined(_M_IX86)) 
#define _MIR_SDR_QUALIFIER __declspec(dllimport)
#elif defined(STATIC_LIB) || defined(__GNUC__) 
#define _MIR_SDR_QUALIFIER
#endif
#endif  // _MIR_SDR_QUALIFIER

// Application code should check that it is compiled against the same API version
// mir_sdr_ApiVersion() returns the API version 
#define MIR_SDR_API_VERSION   (float)(1.1)

#if defined(__GNUC__) && !defined(__i386__) && !defined(__x86_64__)
// Android requires a mechanism to request info from Java application
typedef enum
{
  mir_sdr_GetFd              = 0,
  mir_sdr_FreeFd             = 1,
  mir_sdr_DevNotFound        = 2,
  mir_sdr_DevRemoved         = 3
} mir_sdr_JavaReqT;

typedef int (*mir_sdr_SendJavaReq_t)(mir_sdr_JavaReqT cmd);
#endif

typedef enum
{
  mir_sdr_Success            = 0,
  mir_sdr_Fail               = 1,
  mir_sdr_InvalidParam       = 2,
  mir_sdr_OutOfRange         = 3,
  mir_sdr_GainUpdateError    = 4,
  mir_sdr_RfUpdateError      = 5,
  mir_sdr_FsUpdateError      = 6,
  mir_sdr_HwError            = 7,
  mir_sdr_AliasingError      = 8,
  mir_sdr_AlreadyInitialised = 9,
  mir_sdr_NotInitialised     = 10
} mir_sdr_ErrT;

typedef enum
{
  mir_sdr_BW_0_200 = 200,
  mir_sdr_BW_0_300 = 300,
  mir_sdr_BW_0_600 = 600,
  mir_sdr_BW_1_536 = 1536,
  mir_sdr_BW_5_000 = 5000,
  mir_sdr_BW_6_000 = 6000,
  mir_sdr_BW_7_000 = 7000,
  mir_sdr_BW_8_000 = 8000
} mir_sdr_Bw_MHzT;

typedef enum
{
  mir_sdr_IF_Zero  = 0,
  mir_sdr_IF_0_450 = 450,
  mir_sdr_IF_1_620 = 1620,
  mir_sdr_IF_2_048 = 2048
} mir_sdr_If_kHzT;

// Dll function prototypes
typedef mir_sdr_ErrT (*mir_sdr_Init_t)(int gRdB, double fsMHz, double rfMHz, mir_sdr_Bw_MHzT bwType, mir_sdr_If_kHzT ifType, int *samplesPerPacket);
typedef mir_sdr_ErrT (*mir_sdr_Uninit_t)(void);
typedef mir_sdr_ErrT (*mir_sdr_ReadPacket_t)(short *xi, short *xq, unsigned int *firstSampleNum, int *grChanged, int *rfChanged, int *fsChanged);
typedef mir_sdr_ErrT (*mir_sdr_SetRf_t)(double drfHz, int abs, int syncUpdate);
typedef mir_sdr_ErrT (*mir_sdr_SetFs_t)(double dfsHz, int abs, int syncUpdate, int reCal);
typedef mir_sdr_ErrT (*mir_sdr_SetGr_t)(int gRdB, int abs, int syncUpdate);
typedef mir_sdr_ErrT (*mir_sdr_SetGrParams_t)(int minimumGr, int lnaGrThreshold);
typedef mir_sdr_ErrT (*mir_sdr_SetDcMode_t)(int dcCal, int speedUp);
typedef mir_sdr_ErrT (*mir_sdr_SetDcTrackTime_t)(int trackTime);
typedef mir_sdr_ErrT (*mir_sdr_SetSyncUpdateSampleNum_t)(unsigned int sampleNum);
typedef mir_sdr_ErrT (*mir_sdr_SetSyncUpdatePeriod_t)(unsigned int period);
typedef mir_sdr_ErrT (*mir_sdr_ApiVersion_t)(float *version);   
typedef mir_sdr_ErrT (*mir_sdr_ResetUpdateFlags_t)(int resetGainUpdate, int resetRfUpdate, int resetFsUpdate);   
#if defined(__GNUC__) && !defined(__i386__) && !defined(__x86_64__)
typedef mir_sdr_ErrT (*mir_sdr_SetJavaReqCallback_t)(mir_sdr_SendJavaReq_t sendJavaReq);   
#endif

// API function definitions
#ifdef __cplusplus
extern "C"
{
#endif

   _MIR_SDR_QUALIFIER mir_sdr_ErrT mir_sdr_Init(int gRdB, double fsMHz, double rfMHz, mir_sdr_Bw_MHzT bwType, mir_sdr_If_kHzT ifType, int *samplesPerPacket);
   _MIR_SDR_QUALIFIER mir_sdr_ErrT mir_sdr_Uninit(void);
   _MIR_SDR_QUALIFIER mir_sdr_ErrT mir_sdr_ReadPacket(short *xi, short *xq, unsigned int *firstSampleNum, int *grChanged, int *rfChanged, int *fsChanged);
   _MIR_SDR_QUALIFIER mir_sdr_ErrT mir_sdr_SetRf(double drfHz, int abs, int syncUpdate);
   _MIR_SDR_QUALIFIER mir_sdr_ErrT mir_sdr_SetFs(double dfsHz, int abs, int syncUpdate, int reCal);
   _MIR_SDR_QUALIFIER mir_sdr_ErrT mir_sdr_SetGr(int gRdB, int abs, int syncUpdate);
   _MIR_SDR_QUALIFIER mir_sdr_ErrT mir_sdr_SetGrParams(int minimumGr, int lnaGrThreshold);
   _MIR_SDR_QUALIFIER mir_sdr_ErrT mir_sdr_SetDcMode(int dcCal, int speedUp);
   _MIR_SDR_QUALIFIER mir_sdr_ErrT mir_sdr_SetDcTrackTime(int trackTime);
   _MIR_SDR_QUALIFIER mir_sdr_ErrT mir_sdr_SetSyncUpdateSampleNum(unsigned int sampleNum);
   _MIR_SDR_QUALIFIER mir_sdr_ErrT mir_sdr_SetSyncUpdatePeriod(unsigned int period);
   _MIR_SDR_QUALIFIER mir_sdr_ErrT mir_sdr_ApiVersion(float *version);    // Called by application to retrieve version of API used to create Dll
   _MIR_SDR_QUALIFIER mir_sdr_ErrT mir_sdr_ResetUpdateFlags(int resetGainUpdate, int resetRfUpdate, int resetFsUpdate);    
#if defined(__GNUC__) && !defined(__i386__) && !defined(__x86_64__)
   // This function provides a machanism for the Java application to set
   // the callback function used to send request to it
   _MIR_SDR_QUALIFIER mir_sdr_ErrT mir_sdr_SetJavaReqCallback(mir_sdr_SendJavaReq_t sendJavaReq);   
#endif

#ifdef __cplusplus
}
#endif

#endif //MIR_SDR_H
