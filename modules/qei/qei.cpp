#include "qei.h"

/***********************************************************************
                MODULE CONFIGURATION AND CREATION FROM JSON     
************************************************************************/
std::shared_ptr<Module> QEI::create(const JsonObject& config, Remora* instance) 
{
    const char* comment = config["Comment"];
    printf("%s\n",comment);

    int pv = config["PV[i]"];
    int dataBit = config["Data Bit"];
    const char* index = config["Enable Index"];

    printf("Creating QEI, hardware quadrature encoder interface\n");

    volatile float* ptrProcessVariable = &instance->getTxData()->processVariable[pv];
	volatile uint16_t* ptrInputs = &instance->getTxData()->inputs;

    if (!strcmp(index,"True"))
    {
        printf("  Encoder has index\n");
        return std::make_unique<QEI>(*ptrProcessVariable, *ptrInputs, dataBit);
    }
    else
    {
        return std::make_unique<QEI>(*ptrProcessVariable);
    }
}

/***********************************************************************
*                METHOD DEFINITIONS                                    *
************************************************************************/

QEI::QEI(volatile float &ptrEncoderCount) :
	ptrEncoderCount(&ptrEncoderCount)
{
    //qei = new QEIdriver();
    this->hasIndex = false;
}

QEI::QEI(volatile float &ptrEncoderCount, volatile uint16_t &ptrData, int bitNumber) :
	ptrEncoderCount(&ptrEncoderCount),
    ptrData(&ptrData),
    bitNumber(bitNumber)
{
    //qei = new QEIdriver(true);
    this->hasIndex = true;
    this->indexPulse = 100;                             
	this->count = 0;								    
    this->pulseCount = 0;                               
    this->mask = 1 << this->bitNumber;
}


void QEI::update()
{
    //this->count = this->qei->get();
    this->count = 0;

    if (this->hasIndex)                                     // we have an index pin
    {
        // // handle index, index pulse and pulse count
        // if (this->qei->indexDetected && (this->pulseCount == 0))    // index interrupt occured: rising edge on index pulse
        // {
        //     *(this->ptrEncoderCount) = this->qei->indexCount;
        //     this->pulseCount = this->indexPulse;        
        //     *(this->ptrData) |= this->mask;                 // set bit in data source high
        // }
        // else if (this->pulseCount > 0)                      // maintain both index output and encoder count for the latch period
        // {
        //     this->qei->indexDetected = false;
        //     this->pulseCount--;                             // decrement the counter
        // }
        // else
        // {
        //     *(this->ptrData) &= ~this->mask;                // set bit in data source low
        //     *(this->ptrEncoderCount) = this->count;         // update encoder count
        // }
    }
    else
    {
        *(this->ptrEncoderCount) = this->count;             // update encoder count
    }
}
