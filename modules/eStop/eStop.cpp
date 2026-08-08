#include "eStop.h"

/***********************************************************************
                MODULE CONFIGURATION AND CREATION FROM JSON     
************************************************************************/

std::shared_ptr<Module> eStop::create(const JsonObject& config, Remora* instance)
{
    const char* comment = config["Comment"];
    const char* pin = config["Pin"];
	const char* invert = config["Invert"];
	bool inv = !strcmp(invert, "True");

    printf("%s\n",comment);
    printf("\nCreating eStop at pin %s\n", pin);

    return std::make_unique<eStop>(*instance->getTxData(), pin, inv);
}


/***********************************************************************
                METHOD DEFINITIONS
************************************************************************/
eStop::eStop(volatile txData_t& _ptrTxData, std::string _portAndPin, bool _invert) :
    ptrTxData(&_ptrTxData),
	portAndPin(_portAndPin),
    pin(std::make_unique<Pin>(portAndPin, INPUT)),
    invert(_invert)
{
}

void eStop::update()
{
    bool pinState = pin->get();

    if (invert) 
    {
        pinState = !pinState;
    }
    if (pinState)
    {        
        ptrTxData->header = Config::pruEstop;
    }
    else 
    {
        ptrTxData->header = Config::pruData;
    }
}

void eStop::slowUpdate()
{
	return;
}