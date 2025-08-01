#ifndef COMMSINTERFACE_H
#define COMMSINTERFACE_H

#include <functional>
#include "../data.h"
#include "../modules/module.h"

class CommsInterface : public Module {
private:

protected:
	std::function<void(bool)> dataCallback;

public:
	volatile rxData_t*  		ptrRxData;
	volatile txData_t*  		ptrTxData;

	CommsInterface();

	virtual void init(void);
	virtual void start(void);
	virtual void tasks(void);

	virtual uint8_t read_byte(void);
	virtual uint8_t write_byte(uint8_t);
	virtual void DMA_write(uint8_t*, uint16_t);
	virtual void DMA_read(uint8_t*, uint16_t);

    void setDataCallback(const std::function<void(bool)>& callback) {
        dataCallback = callback;
    }
};

#endif
