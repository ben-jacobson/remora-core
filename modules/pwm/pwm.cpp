#include "pwm.h"

#define PID_PWM_MAX 256		// 8 bit resolution

/***********************************************************************
                MODULE CONFIGURATION AND CREATION FROM JSON     
************************************************************************/
std::shared_ptr<Module> PWM::create(const JsonObject& config, Remora* instance)
{
    PWM* new_pwm;
    volatile float*     variable_pointers[Config::variables];

    int sp = config["SP[i]"];  // when reading this value off the rx buffer, it will return the duty cycle.  
    int pwmMax = config["PWM Max"];
    const char* pin = config["PWM Pin"];
    const char* hardware = config["Hardware PWM"];
    const char* variable = config["Variable Freq"]; // by default all PWMs are variable.
    int period_sp = config["Period SP[i]"];
    int period_us = config["Period us"];
    //const char* comment = config["Comment"];

    //printf("\n%s\n",comment);
    printf("Creating PWM at pin %s\n", pin);

    /*printf("SP[i]: %d\n" // helps with a bit of debugging
       "PWM Max: %d\n"
       "PWM Pin: %s\n"
       "Hardware PWM: %s\n"
       "Variable Freq: %s\n"
       "Period SP[i]: %d\n"
       "Period us: %d\n",
       sp, pwmMax, pin, hardware, variable, period_sp, period_us);*/
   
    // Create pointers for set point variables

    variable_pointers[sp] = &instance->getRxData()->setPoint[sp];  // sp value will store the duty cycle.  
    variable_pointers[period_sp] = &instance->getRxData()->setPoint[period_sp]; // todo - if this isn't enabled what does it do, see if we can check for errors. 
    
    if (!strcmp(hardware, "False")) // Software PWM
    {
        printf("Software PWM not yet supported\n");
    }

    bool variable_freq = !strcmp(variable, "True");
    printf("PWM variable_freq = %d\n", variable_freq);

    //new_pwm = new PWM(*variable_pointers[period_sp], *variable_pointers[sp], variable_freq, period_us, pwmMax, pin);   
    //new_pwm->setPwmMax(pwmMax);
    //servoThread->registerModule(new_pwm);
	return std::make_unique<PWM>(*variable_pointers[period_sp], *variable_pointers[sp], variable_freq, period_us, pwmMax, pin);
}

/***********************************************************************
                METHOD DEFINITIONS
************************************************************************/

PWM::PWM(volatile float &ptrPwmPeriod, volatile float &ptrPwmPulseWidth, bool variable_freq, int fixed_period_us, int pwmMax, std::string pin):
    ptrPwmPeriod(&ptrPwmPeriod),
    variable_freq(variable_freq),
    ptrPwmPulseWidth(&ptrPwmPulseWidth),
    pwmMax(pwmMax),
    pin(pin)
{
    printf("Creating variable frequency Hardware PWM at pin %s\n", this->pin.c_str());

    // set initial period and pulse width
    if (this->variable_freq == true)
    {
        this->pwmPeriod_us = *(this->ptrPwmPeriod);
    }
    else 
    {
        this->pwmPeriod_us = fixed_period_us;    
    }

    if (this->pwmPeriod_us < 1)
    {
        this->pwmPeriod_us = DEFAULT_PWM_PERIOD;
    }

    this->pwmPulseWidth = *(this->ptrPwmPulseWidth);
    this->pwmPulseWidth_us = (this->pwmPeriod_us * this->pwmPulseWidth) / 100.0;

    setPwmMax(pwmMax);
    
    hardware_PWM = new HardwarePWM(this->pwmPeriod_us, this->pwmPulseWidth_us, this->pin); 
}


float PWM::getPwmPeriod(void) { return pwmPeriod_us; }
float PWM::getPwmPulseWidth(void) { return pwmPulseWidth; }
int PWM::getPwmPulseWidth_us(void) { return pwmPulseWidth_us; }
void PWM::setPwmMax(int pwmMax) { this->pwmMax = pwmMax; }

void PWM::update()
{
    if (this->variable_freq == true) 
    {    
        if (*(this->ptrPwmPeriod) != 0 && (*(this->ptrPwmPeriod) != this->pwmPeriod_us))
        {
            // PWM period has changed
            this->pwmPeriod_us = *(this->ptrPwmPeriod);
            this->hardware_PWM->change_period(this->pwmPeriod_us);

            // force pulse width update by triggering the next if block.
            this->pwmPulseWidth = 0;
        }
    }

    if (*(this->ptrPwmPulseWidth) != this->pwmPulseWidth)
    {
        // PWM duty has changed
        this->pwmPulseWidth = *(this->ptrPwmPulseWidth);

        // clamp the percentage in case LinuxCNC sends something other than 0-100%
        if (this->pwmPulseWidth <= 0) 
        {
            this->pwmPulseWidth = 0;
        }
        if (this->pwmPulseWidth > 100)
        {
            this->pwmPulseWidth = 100;
        }

        // Convert duty cycle % to us and update in hardware. 
        // First check if the the duty cycle has exceeded PWM Max 1-256, and substitute the value if so.
        // this->pwmPulseWidth and this->ptrPwmPulseWidth need to keep their values intact or else update method will run continuously.

        if (this->pwmMax > 0 && (this->pwmPulseWidth / 100) * PWMMAX > this->pwmMax)
        {
            int capped_pwm = (this->pwmMax * 100) / PWMMAX;
            this->pwmPulseWidth_us = (this->pwmPeriod_us * capped_pwm) / 100.0;
        }
        else 
        {
            this->pwmPulseWidth_us = (this->pwmPeriod_us * this->pwmPulseWidth) / 100.0;
        }
        this->hardware_PWM->change_pulsewidth(this->pwmPulseWidth_us);
    } 
}

void PWM::slowUpdate()
{
	return;
}