/**
  ******************************************************************************
  * @file    stm8s_flash.c
  * @author  MCD Application Team
  * @version V2.2.0
  * @date    30-September-2014
  * @brief   This file contains all the functions for the FLASH peripheral.
   ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2014 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm8s_flash.h"

/** @addtogroup STM8S_StdPeriph_Driver
  * @{
  */
/**
@code
 This driver provides functions to configure and program the Flash memory of all
 STM8S devices.

 It includes as well functions that can be either executed from RAM or not, and
 other functions that must be executed from RAM otherwise useless.

 The table below lists the functions that can be executed from RAM.

 +--------------------------------------------------------------------------------|
 |   Functions prototypes      |    RAM execution            |     Comments       |
 ---------------------------------------------------------------------------------|
 |                             | Mandatory in case of block  | Can be executed    |
 | FLASH_WaitForLastOperation  | Operation:                  | from Flash in case |
 |                             | - Block programming         | of byte and word   |
 |                             | - Block erase               | Operations         |
 |--------------------------------------------------------------------------------|
 | FLASH_ProgramBlock          |       Exclusively           | useless from Flash |
 |--------------------------------------------------------------------------------|
 | FLASH_EraseBlock            |       Exclusively           | useless from Flash |
 |--------------------------------------------------------------------------------|

 To be able to execute functions from RAM several steps have to be followed.
 These steps may differ from one toolchain to another.
 A detailed description is available below within this driver.
 You can also refer to the FLASH examples provided within the
 STM8S_StdPeriph_Lib package.

@endcode
*/


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define FLASH_CLEAR_BYTE    ((uint8_t)0x00)
#define FLASH_SET_BYTE      ((uint8_t)0xFF)
#define OPERATION_TIMEOUT   ((uint16_t)0xFFFF)
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private Constants ---------------------------------------------------------*/

/** @addtogroup FLASH_Public_functions
  * @{
  */

/**
  * @brief  Unlocks the program or data EEPROM memory
  * @param  FLASH_MemType : Memory type to unlock
  *         This parameter can be a value of @ref FLASH_MemType_TypeDef
  * @retval None
  */
void FLASH_Unlock(FLASH_MemType_TypeDef FLASH_MemType)
{
  /* Check parameter */
  assert_param(IS_MEMORY_TYPE_OK(FLASH_MemType));
  
  /* Unlock program memory */
  if(FLASH_MemType == FLASH_MEMTYPE_PROG)
  {
    FLASH->PUKR = FLASH_RASS_KEY1;
    FLASH->PUKR = FLASH_RASS_KEY2;
  }
  /* Unlock data memory */
  else
  {
    FLASH->DUKR = FLASH_RASS_KEY2; /* Warning: keys are reversed on data memory !!! */
    FLASH->DUKR = FLASH_RASS_KEY1;
  }
}

/**
  * @brief  Locks the program or data EEPROM memory
  * @param  FLASH_MemType : Memory type
  *         This parameter can be a value of @ref FLASH_MemType_TypeDef
  * @retval None
  */
void FLASH_Lock(FLASH_MemType_TypeDef FLASH_MemType)
{
  /* Check parameter */
  assert_param(IS_MEMORY_TYPE_OK(FLASH_MemType));
  
  /* Lock memory */
  FLASH->IAPSR &= (uint8_t)FLASH_MemType;
}

/**
  * @brief  DeInitializes the FLASH registers to their default reset values.
  * @param  None
  * @retval None
  */
void FLASH_DeInit(void)
{
  FLASH->CR1 = FLASH_CR1_RESET_VALUE;
  FLASH->CR2 = FLASH_CR2_RESET_VALUE;
  FLASH->NCR2 = FLASH_NCR2_RESET_VALUE;
  FLASH->IAPSR &= (uint8_t)(~FLASH_IAPSR_DUL);
  FLASH->IAPSR &= (uint8_t)(~FLASH_IAPSR_PUL);
  (void) FLASH->IAPSR; /* Reading of this register causes the clearing of status flags */
}

/**
  * @brief  Enables or Disables the Flash interrupt mode
  * @param  NewState : The new state of the flash interrupt mode
  *         This parameter can be a value of @ref FunctionalState enumeration.
  * @retval None
  */
void FLASH_ITConfig(FunctionalState NewState)
{
  /* Check parameter */
  assert_param(IS_FUNCTIONALSTATE_OK(NewState));
  
  if(NewState != DISABLE)
  {
    FLASH->CR1 |= FLASH_CR1_IE; /* Enables the interrupt sources */
  }
  else
  {
    FLASH->CR1 &= (uint8_t)(~FLASH_CR1_IE); /* Disables the interrupt sources */
  }
}

/**
  * @brief  Erases one byte in the program or data EEPROM memory
  * @note   PointerAttr define is declared in the stm8s.h file to select if 
  *         the pointer will be declared as near (2 bytes) or far (3 bytes).
  * @param  Address : Address of the byte to erase
  * @retval None
  */
void FLASH_EraseByte(uint32_t Address)
{
  /* Check parameter */
  assert_param(IS_FLASH_ADDRESS_OK(Address));
  
  /* Erase byte */
  *(PointerAttr uint8_t*) (MemoryAddressCast)Address = FLASH_CLEAR_BYTE; 
}

/**
  * @brief  Programs one byte in program or data EEPROM memory
  * @note   PointerAttr define is declared in the stm8s.h file to select if 
  *         the pointer will be declared as near (2 bytes) or far (3 bytes).
  * @param  Address : Address where the byte will be programmed
  * @param  Data : Value to be programmed
  * @retval None
  */
void FLASH_ProgramByte(uint32_t Address, uint8_t Data)
{
  /* Check parameters */
  assert_param(IS_FLASH_ADDRESS_OK(Address));
  *(PointerAttr uint8_t*) (MemoryAddressCast)Address = Data;
}

/**
  * @brief  Reads any byte from flash memory
  * @note   PointerAttr define is declared in the stm8s.h file to select if 
  *         the pointer will be declared as near (2 bytes) or far (3 bytes).
  * @param  Address : Address to read
  * @retval Value of the byte
  */
uint8_t FLASH_ReadByte(uint32_t Address)
{
  /* Check parameter */
  assert_param(IS_FLASH_ADDRESS_OK(Address));
  
  /* Read byte */
  return(*(PointerAttr uint8_t *) (MemoryAddressCast)Address); 
}

/**
  * @brief  Programs one word (4 bytes) in program or data EEPROM memory
  * @note   PointerAttr define is declared in the stm8s.h file to select if 
  *         the pointer will be declared as near (2 bytes) or far (3 bytes).
  * @param  Address : The address where the data will be programmed
  * @param  Data : Value to be programmed
  * @retval None
  */
void FLASH_ProgramWord(uint32_t Address, uint32_t Data)
{
  /* Check parameters */
  assert_param(IS_FLASH_ADDRESS_OK(Address));
  
  /* Enable Word Write Once */
  FLASH->CR2 |= FLASH_CR2_WPRG;
  FLASH->NCR2 &= (uint8_t)(~FLASH_NCR2_NWPRG);
  
  /* Write one byte - from lowest address*/
  *((PointerAttr uint8_t*)(MemoryAddressCast)Address)       = *((uint8_t*)(&Data));
  /* Write one byte*/
  *(((PointerAttr uint8_t*)(MemoryAddressCast)Address) + 1) = *((uint8_t*)(&Data)+1); 
  /* Write one byte*/    
  *(((PointerAttr uint8_t*)(MemoryAddressCast)Address) + 2) = *((uint8_t*)(&Data)+2); 
  /* Write one byte - from higher address*/
  *(((PointerAttr uint8_t*)(MemoryAddressCast)Address) + 3) = *((uint8_t*)(&Data)+3); 
}


/**
  * @brief  Select the Flash behaviour in low power mode
  * @param  FLASH_LPMode Low power mode selection
  *         This parameter can be any of the @ref FLASH_LPMode_TypeDef values.
  * @retval None
  */
void FLASH_SetLowPowerMode(FLASH_LPMode_TypeDef FLASH_LPMode)
{
  /* Check parameter */
  assert_param(IS_FLASH_LOW_POWER_MODE_OK(FLASH_LPMode));
  
  /* Clears the two bits */
  FLASH->CR1 &= (uint8_t)(~(FLASH_CR1_HALT | FLASH_CR1_AHALT)); 
  
  /* Sets the new mode */
  FLASH->CR1 |= (uint8_t)FLASH_LPMode; 
}

/**
  * @brief  Sets the fixed programming time
  * @param  FLASH_ProgTime Indicates the programming time to be fixed
  *         This parameter can be any of the @ref FLASH_ProgramTime_TypeDef values.
  * @retval None
  */
void FLASH_SetProgrammingTime(FLASH_ProgramTime_TypeDef FLASH_ProgTime)
{
  /* Check parameter */
  assert_param(IS_FLASH_PROGRAM_TIME_OK(FLASH_ProgTime));
  
  FLASH->CR1 &= (uint8_t)(~FLASH_CR1_FIX);
  FLASH->CR1 |= (uint8_t)FLASH_ProgTime;
}

/**
  * @brief  Returns the Flash behaviour type in low power mode
  * @param  None
  * @retval FLASH_LPMode_TypeDef Flash behaviour type in low power mode
  */
FLASH_LPMode_TypeDef FLASH_GetLowPowerMode(void)
{
  return((FLASH_LPMode_TypeDef)(FLASH->CR1 & (uint8_t)(FLASH_CR1_HALT | FLASH_CR1_AHALT)));
}

/**
  * @brief  Returns the fixed programming time
  * @param  None
  * @retval FLASH_ProgramTime_TypeDef Fixed programming time value
  */
FLASH_ProgramTime_TypeDef FLASH_GetProgrammingTime(void)
{
  return((FLASH_ProgramTime_TypeDef)(FLASH->CR1 & FLASH_CR1_FIX));
}

/**
  * @brief  Returns the Boot memory size in bytes
  * @param  None
  * @retval Boot memory size in bytes
  */
uint32_t FLASH_GetBootSize(void)
{
  uint32_t temp = 0;
  
  /* Calculates the number of bytes */
  temp = (uint32_t)((uint32_t)FLASH->FPR * (uint32_t)512);
  
  /* Correction because size of 127.5 kb doesn't exist */
  if(FLASH->FPR == 0xFF)
  {
    temp += 512;
  }
  
  /* Return value */
  return(temp);
}

/**
  * @brief  Checks whether the specified SPI flag is set or not.
  * @param  FLASH_FLAG : Specifies the flag to check.
  *         This parameter can be any of the @ref FLASH_Flag_TypeDef enumeration.
  * @retval FlagStatus : Indicates the state of FLASH_FLAG.
  *         This parameter can be any of the @ref FlagStatus enumeration.
  * @note   This function can clear the EOP, WR_PG_DIS flags in the IAPSR register.
  */
FlagStatus FLASH_GetFlagStatus(FLASH_Flag_TypeDef FLASH_FLAG)
{
  FlagStatus status = RESET;
  /* Check parameters */
  assert_param(IS_FLASH_FLAGS_OK(FLASH_FLAG));
  
  /* Check the status of the specified FLASH flag */
  if((FLASH->IAPSR & (uint8_t)FLASH_FLAG) != (uint8_t)RESET)
  {
    status = SET; /* FLASH_FLAG is set */
  }
  else
  {
    status = RESET; /* FLASH_FLAG is reset*/
  }
  
  /* Return the FLASH_FLAG status */
  return status;
}


#if defined (_COSMIC_) && defined (RAM_EXECUTION)
 /* End of FLASH_CODE section */
 #pragma section ()
#endif /* _COSMIC_ && RAM_EXECUTION */


/**
  * @}
  */
  
/**
  * @}
  */
  

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
