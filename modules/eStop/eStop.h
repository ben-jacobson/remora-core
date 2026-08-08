#ifndef ESTOP_H
#define ESTOP_H

#include "../../remora.h"
#include "../../modules/module.h"
#include "../../../remora-hal/pin/pin.h"

class eStop : public Module
{
	private: 
	    volatile txData_t*  ptrTxData;

		std::string portAndPin;
        std::unique_ptr<Pin> pin;  
		
		bool invert; 

	public:
		static std::shared_ptr<Module> create(const JsonObject& config, Remora* instance);
		eStop(volatile txData_t& _ptrTxData, std::string _portAndPin, bool _invert);

		virtual void update(void);
		virtual void slowUpdate(void);
};

#endif