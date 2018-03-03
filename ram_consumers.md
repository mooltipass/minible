## [](#header-1) Mooltipass RAM Consumers
Here we list the biggest RAM consumers and explain why they're needed.

| size | name | description |
|:-----|:-----|:------------|
| ~212B | acc_descriptor | 32 XYZ accelerometer data DMA read |
| 224B | dma(_writeback)_descriptors | DMA (writeback) descriptors |
| 128B | custom_fs_temp_string1 | buffer for external flash string reads |
| ~236B | oled_descriptor | single row buffer, current font header |
| 128B | custom_fs_flash_header | external bundle flash header copy |
| 1072B | aux_mcu_message_buffer1/2 | message buffers for aux mcu comms |
