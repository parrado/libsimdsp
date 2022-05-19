



//Size of shared memory block
#define NSHARED (2048)

#define SETTINGS_OFFSET (NSHARED/2)

#define SAMPLING_RATE  (SETTINGS_OFFSET+0)
#define INPUT_SOURCE     (SETTINGS_OFFSET+1)
#define SINE_FREQUENCY     (SETTINGS_OFFSET+2)
#define NOISE_FLAG (SETTINGS_OFFSET+3)
#define NOISE_POWER (SETTINGS_OFFSET+4)
void main_client();
void writeInputSignal(short *x,int n);
void writeOutputSignal(short *x,int n);
void writeSamplingRate(double fs);
double readInputSource();
double readSamplingRate();

double readSineFrequency();

double readNoiseFlag();

double readNoisePower();
