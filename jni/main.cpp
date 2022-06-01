#include "sharedmem.h"
#include <unistd.h>
void __attribute__((weak)) dsp_setup();

int main(){
main_client();
dsp_setup();

while(true){
usleep(10000);
}

}
