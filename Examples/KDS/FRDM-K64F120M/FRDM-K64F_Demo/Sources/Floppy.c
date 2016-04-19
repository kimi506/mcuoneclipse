/*
 * Floppy.c
 *
 *  Created on: 14.04.2016
 *      Author: Erich Styger
 */
#include "Platform.h"
#if PL_HAS_FLOPPY
#include "Floppy.h"
#include "FRTOS1.h"
#include "UTIL1.h"
#include "TI1.h"
#include "MidiMusic.h"
#include "CLS1.h"
#include "WAIT1.h"

#include "En0.h"
#include "Dir0.h"
#include "Step0.h"

#include "En1.h"
#include "Dir1.h"
#include "Step1.h"

#include "En2.h"
#include "Dir2.h"
#include "Step2.h"

/* songs:
 * http://www.makeuseof.com/tag/8-floppy-disk-drive-music-videos/
 * http://makezine.com/2014/08/18/how-to-play-music-using-floppy-drives/
 * http://hackaday.com/2014/01/05/the-most-beautiful-floppy-disk-jukebox-ever/
 * http://www.schoar.de/tinkering/rumblerail/
 */

/* green: enable (low active)
 * white: direction
 * green: step
 */
#define FLOPPY_NOTE_OFFSET     (3*12)  /* Octave is 12 */
#define FLOPPY_CHANGE_DIRECTION  1

#define FLOPPY_NOF_NOTES  128
static const uint16_t FLOPPY_NoteTicks[FLOPPY_NOF_NOTES] = {
    1030  , // Note 0
    1022  , // Note 1
    1014  , // Note 2
    1007  , // Note 3
    999 , // Note 4
    991 , // Note 5
    983 , // Note 6
    975 , // Note 7
    967 , // Note 8
    959 , // Note 9
    951 , // Note 10
    944 , // Note 11
    936 , // Note 12
    928 , // Note 13
    920 , // Note 14
    912 , // Note 15
    904 , // Note 16
    896 , // Note 17
    888 , // Note 18
    881 , // Note 19
    873 , // Note 20
    865 , // Note 21
    857 , // Note 22
    849 , // Note 23
    841 , // Note 24
    833 , // Note 25
    825 , // Note 26
    818 , // Note 27
    810 , // Note 28
    802 , // Note 29
    794 , // Note 30
    786 , // Note 31
    778 , // Note 32
    770 , // Note 33
    762 , // Note 34
    755 , // Note 35
    747 , // Note 36
    739 , // Note 37
    731 , // Note 38
    723 , // Note 39
    715 , // Note 40
    707 , // Note 41
    699 , // Note 42
    692 , // Note 43
    684 , // Note 44
    676 , // Note 45
    668 , // Note 46
    660 , // Note 47
    652 , // Note 48
    644 , // Note 49
    636 , // Note 50
    629 , // Note 51
    621 , // Note 52
    613 , // Note 53
    605 , // Note 54
    597 , // Note 55
    589 , // Note 56
    581 , // Note 57
    573 , // Note 58
    566 , // Note 59
    558 , // Note 60
    550 , // Note 61
    542 , // Note 62
    534 , // Note 63
    526 , // Note 64
    518 , // Note 65
    510 , // Note 66
    503 , // Note 67
    495 , // Note 68
    487 , // Note 69
    479 , // Note 70
    471 , // Note 71
    463 , // Note 72
    455 , // Note 73
    447 , // Note 74
    440 , // Note 75
    432 , // Note 76
    424 , // Note 77
    416 , // Note 78
    408 , // Note 79
    400 , // Note 80
    392 , // Note 81
    384 , // Note 82
    377 , // Note 83
    369 , // Note 84
    361 , // Note 85
    353 , // Note 86
    345 , // Note 87
    337 , // Note 88
    329 , // Note 89
    321 , // Note 90
    314 , // Note 91
    306 , // Note 92
    298 , // Note 93
    290 , // Note 94
    282 , // Note 95
    274 , // Note 96
    266 , // Note 97
    258 , // Note 98
    251 , // Note 99
    243 , // Note 100
    235 , // Note 101
    227 , // Note 102
    219 , // Note 103
    211 , // Note 104
    203 , // Note 105
    195 , // Note 106
    188 , // Note 107
    180 , // Note 108
    172 , // Note 109
    164 , // Note 110
    156 , // Note 111
    148 , // Note 112
    140 , // Note 113
    132 , // Note 114
    125 , // Note 115
    117 , // Note 116
    109 , // Note 117
    101 , // Note 118
    93  , // Note 119
    85  , // Note 120
    77  , // Note 121
    69  , // Note 122
    62  , // Note 123
    54  , // Note 124
    46  , // Note 125
    38  , // Note 126
    30   // Note 127
};

#define FLOPPY_NOF_DRIVES  3
#define FLOPPY_MAX_STEPS  80

static void Drv0_Dir(bool forward) { if (forward) {Dir0_SetVal();} else {Dir0_ClrVal();} }
static void Drv0_Step(void) { Step0_SetVal(); WAIT1_Waitus(5); Step0_ClrVal(); }
static void Drv0_StepSetVal(void) { Step0_SetVal(); }
static void Drv0_StepClrVal(void) { Step0_ClrVal(); }
static void Drv0_StepToggle(void) { Step0_NegVal(); }
static void Drv0_Enable(void) { En0_ClrVal(); }
static void Drv0_Disable(void) { En0_SetVal(); }

static void Drv1_Dir(bool forward) { if (forward) {Dir1_SetVal();} else {Dir1_ClrVal();} }
static void Drv1_Step(void) { Step1_SetVal(); WAIT1_Waitus(5); Step1_ClrVal(); }
static void Drv1_StepSetVal(void) { Step1_SetVal(); }
static void Drv1_StepClrVal(void) { Step1_ClrVal(); }
static void Drv1_StepToggle(void) { Step1_NegVal(); }
static void Drv1_Enable(void) { En1_ClrVal(); }
static void Drv1_Disable(void) { En1_SetVal(); }

static void Drv2_Dir(bool forward) { if (forward) {Dir2_SetVal();} else {Dir2_ClrVal();} }
static void Drv2_Step(void) { Step2_SetVal(); WAIT1_Waitus(5); Step2_ClrVal(); }
static void Drv2_StepSetVal(void) { Step2_SetVal(); }
static void Drv2_StepClrVal(void) { Step2_ClrVal(); }
static void Drv2_StepToggle(void) { Step2_NegVal(); }
static void Drv2_Enable(void) { En2_ClrVal(); }
static void Drv2_Disable(void) { En2_SetVal(); }

typedef struct {
  bool forward; /* current direction */
  bool enabled; /* if enabled or not */
  int8_t pos; /* current position, valid 0..FLOPPY_MAX_STEPS */
  uint32_t currentPeriod; /* current period in timer interrupt events. Zero if disabled */
  uint32_t currentTick;
  void(*Dir)(bool forward);
  void(*Step)(void);
  void(*StepSetVal)(void);
  void(*StepClearVal)(void);
  void(*StepToggle)(void);
  void(*Enable)(void);
  void(*Disable)(void);
} FLOPPY_Drive;
typedef FLOPPY_Drive *FLOPPY_DriveHandle;

static FLOPPY_Drive FLOPPY_Drives[FLOPPY_NOF_DRIVES];

uint8_t FLOPPY_Enable(FLOPPY_DriveHandle drive) {
  drive->Enable();
  drive->enabled = TRUE;
  return ERR_OK;
}

uint8_t FLOPPY_Disable(FLOPPY_DriveHandle drive) {
  drive->enabled = FALSE;
  drive->Disable();
  return ERR_OK;
}

uint8_t FLOPPY_SetPos(FLOPPY_DriveHandle drive, int pos) {
  drive->pos = pos;
  return ERR_OK;
}

uint8_t FLOPPY_SetDirection(FLOPPY_DriveHandle drive, bool forward) {
  drive->Dir(forward);
  drive->forward = forward;
  return ERR_OK;
}

uint8_t FLOPPY_Steps(FLOPPY_DriveHandle drive, int steps) {
  //FLOPPY_Enable(drive);
  if (steps>=0) {
    FLOPPY_SetDirection(drive, TRUE); /* go forward */
  } else if (steps<0) {
    FLOPPY_SetDirection(drive, FALSE); /* go backward */
  }
  while(steps!=0) {
#if FLOPPY_CHANGE_DIRECTION
    if (drive->pos==FLOPPY_MAX_STEPS) {
      FLOPPY_SetDirection(drive, FALSE); /* go backward */
    } else if (drive->pos==0) {
      FLOPPY_SetDirection(drive, TRUE); /* go forward */
    }
#endif
    drive->Step();
    if (steps>0) {
      steps--;
    } else {
      steps++;
    }
    if (drive->forward) {
      drive->pos++;
    } else {
      drive->pos--;
    }
    vTaskDelay(pdMS_TO_TICKS(5));
  } /* while */
  //FLOPPY_Disable(drive);
  return ERR_OK;
}

#if 0
void FLOPPY_OneStep(FLOPPY_DriveHandle drive) {
#if FLOPPY_CHANGE_DIRECTION
  if (drive->pos==FLOPPY_MAX_STEPS) {
    FLOPPY_SetDirection(drive, FALSE); /* go backward */
  } else if (drive->pos==0) {
    FLOPPY_SetDirection(drive, TRUE); /* go forward */
  }
#endif
  if (drive->forward) {
    drive->pos++;
  } else {
    drive->pos--;
  }
  drive->Step();
}
#endif

void FLOPPY_OnInterrupt(void) {
  int i;

  for(i=0;i<FLOPPY_NOF_DRIVES;i++) {
    if (FLOPPY_Drives[i].currentPeriod>0) { /* not disabled */
      //FLOPPY_Drives[i].Enable();
      FLOPPY_Drives[i].currentTick++; /* increment tick */
      if (FLOPPY_Drives[i].currentTick>=FLOPPY_Drives[i].currentPeriod) { /* check if expired */
        FLOPPY_Drives[i].StepSetVal(); /* toggle pin ==> High */
        FLOPPY_Drives[i].currentTick = 0; /* reset tick counter */
#if FLOPPY_CHANGE_DIRECTION
        /* change direction if end has been reached */
        if (FLOPPY_Drives[i].pos==FLOPPY_MAX_STEPS) {
          FLOPPY_SetDirection(&FLOPPY_Drives[i], FALSE); /* go backward */
        } else if (FLOPPY_Drives[i].pos==0) {
          FLOPPY_SetDirection(&FLOPPY_Drives[i], TRUE); /* go forward */
        }
#endif
        if (FLOPPY_Drives[i].forward) {
          FLOPPY_Drives[i].pos++;
        } else {
          FLOPPY_Drives[i].pos--;
        }
        WAIT1_Waitus(5);
        FLOPPY_Drives[i].StepClearVal(); /* toggle pin ==> Low */
      }
      //FLOPPY_Drives[i].Disable();
    }
  }
}

void FLOPPY_MIDI_SetBank(uint8_t channel, uint8_t value) {
  /* not supported */
}

void FLOPPY_MIDI_NoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  if (channel>=FLOPPY_NOF_DRIVES) {
    return;
  }
  if (note>=FLOPPY_NOF_NOTES) {
    return;
  }
  note += FLOPPY_NOTE_OFFSET; /* adjust note */
  if (note>FLOPPY_NOF_NOTES-1) { /* make sure we are inside valid range */
    note = FLOPPY_NOF_NOTES-1; /* 0..FLOPPY_NOF_NOTES-1 */
  }
  FLOPPY_Drives[channel].currentPeriod = FLOPPY_NoteTicks[note];
}

void FLOPPY_MIDI_NoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
  if (channel>=FLOPPY_NOF_DRIVES) {
    return;
  }
  FLOPPY_Drives[channel].currentPeriod = 0;
}

void FLOPPY_MIDI_SetInstrument(uint8_t channel, uint8_t instrument) {
  /* not supported */
}

void FLOPPY_MIDI_SetVolume(uint8_t channel, uint8_t volume) {
  /* not supported */
}

void FLOPPY_MIDI_SetPan(uint8_t channel, uint8_t pan) {
  /* not supported */
}

void FLOPPY_MIDI_AllSoundOff(uint8_t channel) {
  if (channel>=FLOPPY_NOF_DRIVES) {
    return;
  }
  FLOPPY_Drives[channel].currentPeriod = 0;
}


uint8_t FLOPPY_InitDrives(void) {
  /* moving head to home position */
  int i, j;

  for(i=0;i<FLOPPY_NOF_DRIVES;i++) {
    /* reset to initial position */
    FLOPPY_Drives[i].Enable();
    FLOPPY_Drives[i].enabled = TRUE;
    FLOPPY_Drives[i].Dir(TRUE);
    FLOPPY_Drives[i].forward = TRUE;
    for(j=0;j<FLOPPY_MAX_STEPS+20;j++) { /* max 400 Hz */
      FLOPPY_Drives[i].StepSetVal();
      vTaskDelay(pdMS_TO_TICKS(1));
      FLOPPY_Drives[i].StepClearVal();
      vTaskDelay(pdMS_TO_TICKS(3));
    }
    FLOPPY_Drives[i].pos = FLOPPY_MAX_STEPS;
//    FLOPPY_Drives[i].Disable();
//    FLOPPY_Drives[i].enabled = FALSE;
  }
  return ERR_OK;
}

static bool play = FALSE;

static void FloppyTask(void *pvParameters) {
  FLOPPY_InitDrives();
  TI1_Enable();
  TI1_EnableEvent();
  for(;;) {
    if (play) {
      play = FALSE;
      MM_Play();
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

static uint8_t PrintStatus(const CLS1_StdIOType *io) {
  int i;
  uint8_t buf[16], buf2[32];

  CLS1_SendStatusStr((uint8_t*)"Floppy", (uint8_t*)"\r\n", io->stdOut);
  for(i=0;i<FLOPPY_NOF_DRIVES;i++) {
    UTIL1_strcpy(buf, sizeof(buf), (uint8_t*)"  drive ");
    UTIL1_strcatNum16u(buf, sizeof(buf), (uint16_t)i);
    UTIL1_chcat(buf, sizeof(buf), ':');
    buf2[0] = '\0';
    if (FLOPPY_Drives[i].enabled) {
      UTIL1_strcat(buf2, sizeof(buf2), (uint8_t*)"enabled  ");
    } else {
      UTIL1_strcat(buf2, sizeof(buf2), (uint8_t*)"disabled ");
    }
    if (FLOPPY_Drives[i].forward) {
      UTIL1_strcat(buf2, sizeof(buf2), (uint8_t*)"fw ");
    } else {
      UTIL1_strcat(buf2, sizeof(buf2), (uint8_t*)"bw ");
    }
    UTIL1_strcatNum16s(buf2, sizeof(buf2), FLOPPY_Drives[i].pos);
    UTIL1_strcat(buf2, sizeof(buf2), (uint8_t*)"\r\n");
    CLS1_SendStatusStr(buf, buf2, io->stdOut);
  }
  return ERR_OK;
}

uint8_t FLOPPY_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io) {
  const uint8_t *p;
  int32_t val;

  if (UTIL1_strcmp((char*)cmd, CLS1_CMD_HELP)==0 || UTIL1_strcmp((char*)cmd, "Floppy help")==0) {
    CLS1_SendHelpStr((unsigned char*)"Floppy", (const unsigned char*)"Group of Floppy commands\r\n", io->stdOut);
    CLS1_SendHelpStr((unsigned char*)"  help|status", (const unsigned char*)"Print help or status information\r\n", io->stdOut);
    CLS1_SendHelpStr((unsigned char*)"  enable <drv>", (const unsigned char*)"Enable drive\r\n", io->stdOut);
    CLS1_SendHelpStr((unsigned char*)"  disable <drv>", (const unsigned char*)"Disable drive\r\n", io->stdOut);
    CLS1_SendHelpStr((unsigned char*)"  dir fw <drv>", (const unsigned char*)"Set drive direction to forward\r\n", io->stdOut);
    CLS1_SendHelpStr((unsigned char*)"  dir bw <drv>", (const unsigned char*)"Set drive direction to backward\r\n", io->stdOut);
    CLS1_SendHelpStr((unsigned char*)"  home", (const unsigned char*)"Move heads to home position\r\n", io->stdOut);
    CLS1_SendHelpStr((unsigned char*)"  step <drv> <steps>", (const unsigned char*)"Perform number of steps (positive/negative)\r\n", io->stdOut);
    CLS1_SendHelpStr((unsigned char*)"  period <drv> <ticks>", (const unsigned char*)"Set period for drive, zero disables it\r\n", io->stdOut);
    CLS1_SendHelpStr((unsigned char*)"  note <drv> <note>", (const unsigned char*)"Play note (0..127)\r\n", io->stdOut);
    CLS1_SendHelpStr((unsigned char*)"  play", (const unsigned char*)"play music\r\n", io->stdOut);
    *handled = TRUE;
    return ERR_OK;
  } else if ((UTIL1_strcmp((char*)cmd, CLS1_CMD_STATUS)==0) || (UTIL1_strcmp((char*)cmd, "Floppy status")==0)) {
    *handled = TRUE;
    return PrintStatus(io);
  } else if (UTIL1_strncmp((char*)cmd, "Floppy enable", sizeof("Floppy enable")-1)==0) {
    *handled = TRUE;
    p = cmd + sizeof("Floppy enable")-1;
    if (UTIL1_xatoi(&p, &val)==ERR_OK && val>=0 && val<FLOPPY_NOF_DRIVES) {
      return FLOPPY_Enable(&FLOPPY_Drives[val]);
    }
    return ERR_FAILED;
  } else if (UTIL1_strncmp((char*)cmd, "Floppy disable", sizeof("Floppy disable")-1)==0) {
    *handled = TRUE;
    p = cmd + sizeof("Floppy disable")-1;
    if (UTIL1_xatoi(&p, &val)==ERR_OK && val>=0 && val<FLOPPY_NOF_DRIVES) {
      return FLOPPY_Disable(&FLOPPY_Drives[val]);
    }
    return ERR_FAILED;
  } else if (UTIL1_strncmp((char*)cmd, "Floppy dir fw", sizeof("Floppy dir fw")-1)==0) {
    *handled = TRUE;
    p = cmd + sizeof("Floppy dir fw")-1;
    if (UTIL1_xatoi(&p, &val)==ERR_OK && val>=0 && val<FLOPPY_NOF_DRIVES) {
      return FLOPPY_SetDirection(&FLOPPY_Drives[val], TRUE);
    }
    return ERR_FAILED;
  } else if (UTIL1_strncmp((char*)cmd, "Floppy dir bw", sizeof("Floppy dir bw")-1)==0) {
    *handled = TRUE;
    p = cmd + sizeof("Floppy dir bw")-1;
    if (UTIL1_xatoi(&p, &val)==ERR_OK && val>=0 && val<FLOPPY_NOF_DRIVES) {
      return FLOPPY_SetDirection(&FLOPPY_Drives[val], FALSE);
    }
    return ERR_FAILED;
  } else if (UTIL1_strcmp((char*)cmd, "Floppy home")==0) {
    *handled = TRUE;
    return FLOPPY_InitDrives();
  } else if (UTIL1_strcmp((char*)cmd, "Floppy play")==0) {
    *handled = TRUE;
    play = TRUE;
    return ERR_OK;
  } else if (UTIL1_strncmp((char*)cmd, "Floppy step", sizeof("Floppy step")-1)==0) {
    *handled = TRUE;
    p = cmd + sizeof("Floppy step")-1;
    if (UTIL1_xatoi(&p, &val)==ERR_OK && val>=0 && val<FLOPPY_NOF_DRIVES) {
      uint8_t drive = val;

      if (UTIL1_xatoi(&p, &val)==ERR_OK) {
        return FLOPPY_Steps(&FLOPPY_Drives[drive], val);
      }
    }
    return ERR_FAILED;
  } else if (UTIL1_strncmp((char*)cmd, "Floppy period", sizeof("Floppy period")-1)==0) {
    *handled = TRUE;
    p = cmd + sizeof("Floppy period")-1;
    if (UTIL1_xatoi(&p, &val)==ERR_OK && val>=0 && val<FLOPPY_NOF_DRIVES) {
      uint8_t drive = val;

      if (UTIL1_xatoi(&p, &val)==ERR_OK) {
        FLOPPY_Drives[drive].currentPeriod = val;
        return ERR_OK;
      }
    }
    return ERR_FAILED;
  } else if (UTIL1_strncmp((char*)cmd, "Floppy note", sizeof("Floppy note")-1)==0) {
    *handled = TRUE;
    p = cmd + sizeof("Floppy note")-1;
    if (UTIL1_xatoi(&p, &val)==ERR_OK && val>=0 && val<FLOPPY_NOF_DRIVES) {
      uint8_t drive = val;

      if (UTIL1_xatoi(&p, &val)==ERR_OK && val<FLOPPY_NOF_NOTES) {
        FLOPPY_Drives[drive].currentPeriod = FLOPPY_NoteTicks[val];
        return ERR_OK;
      }
    }
    return ERR_FAILED;
  }
  return ERR_OK;
}


void FLOPPY_Init(void) {
  int i;
  if (FRTOS1_xTaskCreate(
      FloppyTask,  /* pointer to the task */
      "Floppy", /* task name for kernel awareness debugging */
      configMINIMAL_STACK_SIZE, /* task stack size */
      (void*)NULL, /* optional task startup argument */
      tskIDLE_PRIORITY+1,  /* initial priority */
      (xTaskHandle*)NULL /* optional task handle to create */
    ) != pdPASS) {
  /*lint -e527 */
  for(;;){} /* error! probably out of memory */
    /*lint +e527 */
  }
  for(i=0;i<FLOPPY_NOF_DRIVES;i++) {
    FLOPPY_Drives[i].forward = FALSE;
    FLOPPY_Drives[i].enabled = FALSE;
    FLOPPY_Drives[i].pos = 0;
    FLOPPY_Drives[i].currentPeriod = 0;
    FLOPPY_Drives[i].currentTick = 0;
  }
  FLOPPY_Drives[0].Dir = Drv0_Dir;
  FLOPPY_Drives[0].Disable = Drv0_Disable;
  FLOPPY_Drives[0].Enable = Drv0_Enable;
  FLOPPY_Drives[0].Step = Drv0_Step;
  FLOPPY_Drives[0].StepSetVal = Drv0_StepSetVal;
  FLOPPY_Drives[0].StepClearVal = Drv0_StepClrVal;
  FLOPPY_Drives[0].StepToggle = Drv0_StepToggle;

  FLOPPY_Drives[1].Dir = Drv1_Dir;
  FLOPPY_Drives[1].Disable = Drv1_Disable;
  FLOPPY_Drives[1].Enable = Drv1_Enable;
  FLOPPY_Drives[1].Step = Drv1_Step;
  FLOPPY_Drives[1].StepSetVal = Drv1_StepSetVal;
  FLOPPY_Drives[1].StepClearVal = Drv1_StepClrVal;
  FLOPPY_Drives[1].StepToggle = Drv1_StepToggle;

  FLOPPY_Drives[2].Dir = Drv2_Dir;
  FLOPPY_Drives[2].Disable = Drv2_Disable;
  FLOPPY_Drives[2].Enable = Drv2_Enable;
  FLOPPY_Drives[2].Step = Drv2_Step;
  FLOPPY_Drives[2].StepSetVal = Drv2_StepSetVal;
  FLOPPY_Drives[2].StepClearVal = Drv2_StepClrVal;
  FLOPPY_Drives[2].StepToggle = Drv2_StepToggle;
}
#endif /* PL_HAS_FLOPPY */