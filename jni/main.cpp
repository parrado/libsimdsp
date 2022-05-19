#include "sharedmem.h"
#include <unistd.h>
void __attribute__((weak)) dsp_setup();
void __attribute__((weak)) dsp_loop();

int main(){
main_client();
dsp_setup();

while(true){
dsp_loop();
usleep(10000);
}

}
