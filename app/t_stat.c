/*
 * thermostat.c
 *
 *  Created on: Nov 26, 2022
 *      Author: sep
 */
#include "stm32_regs.h"
#include "stm32f1xx_ll_system.h"
#include "stdbool.h"
#include "serial_handler.h"
#include "appTasks.h"
#include "global.h"

#include "t_stat.h"

/****************************************************************************/
/***        Global Variables                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
static uint16_t tcTemp;
static uint16_t setPoint;
static uint16_t prevSP;
static uint16_t hist;
static uint8_t duty;
static uint16_t workPoint;
static uint8_t tx_buf[16];
static uint8_t rx_buf[16];
static bool runFlag = false;

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
static void regTemp(void);
static uint16_t readTC(void);
static void setDuty(uint16_t duty);

/****************************************************************************
 * regTemp
 *
 * DESCRIPTION:
 *
 */
static void regTemp(void) {

    uint8_t pwmDuty;

    if(runFlag == false){
        return;
    }

    if(setPoint != prevSP){
        prevSP = setPoint;
        workPoint = setPoint - hist;
    }
    tcTemp = readTC();
    if(tcTemp > workPoint){
        if(workPoint > setPoint){
            workPoint = setPoint - hist;
        }
    }
    if(tcTemp < workPoint){
        if(workPoint < setPoint){
            workPoint = setPoint + hist;
        }
    }
    pwmDuty = 0;
    if(tcTemp < workPoint){
        pwmDuty = duty;
    }
    setDuty(pwmDuty);

    if(pwmDuty == 0){
        // turn on fan
        REG_Write(GPIOB__BSRR, (uint32_t)1 << 10);
    }
    else {
        // turn off fan
        REG_Write(GPIOB__BRR, (uint32_t)1 << 10);
    }

    addAppTask(regTemp, 1000);
}

/****************************************************************************
 * readTC
 *
 * DESCRIPTION:
 *
 */
static uint16_t readTC(void) {

    spi_reg_t spi;
    dma_reg_t dma;

    uint16_t tc;

    dma.mem_addr.b.ma = (uint32_t)&rx_buf[0];
    REG_Write(DMA__CMAR2, dma.mem_addr.all);
    dma.mem_addr.b.ma = (uint32_t)&tx_buf[0];
    REG_Write(DMA__CMAR3, dma.mem_addr.all);

    rx_buf[0] = 0x00;
    rx_buf[1] = 0x00;

    tx_buf[0] = 0xAC;
    tx_buf[1] = 0xDC;

    spiFlag = false;

    dma.data_len.b.ndt = 2;
    REG_Write(DMA__CNDTR2, dma.data_len.all);
    REG_Write(DMA__CNDTR3, dma.data_len.all);

    REG_Write(GPIOB__BRR, (uint32_t)1 << 5); // reset CS, (PB5 -> 0)

    // SPI enable
    spi.cr1.all = REG_Read(SPI1__CR1);
    spi.cr1.b.spi_en = 1;
    REG_Write(SPI1__CR1, spi.cr1.all);

    // DMA Rx enable
    dma.ch_cfg.all = REG_Read(DMA__CCR2);
    dma.ch_cfg.b.en = 1;
    REG_Write(DMA__CCR2, dma.ch_cfg.all);

    // DMA Tx enable
    dma.ch_cfg.all = REG_Read(DMA__CCR3);
    dma.ch_cfg.b.en = 1;
    REG_Write(DMA__CCR3, dma.ch_cfg.all);

    tmoTime = 3;
    while((spiFlag == false) && (tmoTime > 0));
    //while(spiFlag == false);

    // DMA Rx disable
    dma.ch_cfg.all = REG_Read(DMA__CCR2);
    dma.ch_cfg.b.en = 0;
    REG_Write(DMA__CCR2, dma.ch_cfg.all);

    // DMA Tx disable
    dma.ch_cfg.all = REG_Read(DMA__CCR3);
    dma.ch_cfg.b.en = 0;
    REG_Write(DMA__CCR3, dma.ch_cfg.all);

    // SPI disable
    spi.cr1.all = REG_Read(SPI1__CR1);
    spi.cr1.b.spi_en = 0;
    REG_Write(SPI1__CR1, spi.cr1.all);

    REG_Write(GPIOB__BSRR, (uint32_t)1 << 5); // set CS, PB5 -> 1

    tc = rx_buf[0];
    tc <<= 8;
    tc |= rx_buf[1];

    if(tc & (1 << 2)){
        tc = 1000 * 4;
    }
    else {
        tc >>= 3;
    }

    return tc;
}

/****************************************************************************
 * readTC
 *
 * DESCRIPTION:
 *
 */
static void setDuty(uint16_t duty) {

    tim_reg_t tim;
    uint16_t compVal = duty * 100;

    tim.ccr.all = REG_Read(TIM2__CCR1);
    if(tim.ccr.b.ccr != compVal){
        tim.ccr.b.ccr = compVal;
        REG_Write(TIM2__CCR1, tim.ccr.all);
    }
}

/****************************************************************************
 * getThermostat
 *
 * DESCRIPTION:
 *
  */
void getThermostat(void) {

    tsRsp_t *rsp;

    rsp = (tsRsp_t *)&slTxBuf[0];
    rsp->runFlag = runFlag;
    rsp->tcTemp = tcTemp;
    rsp->setPoint = setPoint;
    rsp->hist = hist;
    rsp->duty = duty;

    slWriteMsg(SERIAL_GET_THERMOSTAT,
               sizeof(tsRsp_t),
               &slTxBuf[0]);

}

/****************************************************************************
 * setThemostat
 *
 * DESCRIPTION:
 *
  */
void setThermostat(void) {

    tsSet_t *set;

    set = (tsSet_t *)&rxMsg[0];
    setPoint = set->setPoint;
    hist = set->hist;
    duty = set->duty;

    if(set->runFlag == true){
        if(runFlag == false){
            runFlag = true;
            prevSP = setPoint;
            workPoint = setPoint - hist;
            regTemp();
        }
    }
    else {
        runFlag = false;
        setDuty(0);
    }
}


