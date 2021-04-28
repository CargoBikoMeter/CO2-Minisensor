#ifndef LORA_H
#define LORA_H

#define LORA_APP_PORT 3
#define LORA_APP_DATA_SIZE 4

#define LORA_SPREADING_FACTOR  9  // [SF7..SF12]

void lora_setup();
void lora_process();

/*
 * set LoraWan_RGB to 1,the RGB active in loraWan
 * RGB red means sending;
 * RGB green means received done;
 */
#define LoraWan_RGB 1


#endif