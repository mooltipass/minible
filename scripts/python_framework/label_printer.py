#!/usr/bin/env python
# pip install viivakoodi
import brother_ql   
import logging
import barcode
import time
import os
from barcode.writer import ImageWriter
try:
    import Image, ImageDraw, ImageFont      
except ImportError:     
    from PIL import Image, ImageDraw, ImageFont
logging.basicConfig(level='ERROR')
PRINTER_MODEL = "QL-700"

CODE128 = barcode.get_barcode_class('code128')
FONT = "FreeSans.ttf"

ptoptions = {
  "module_width":   0.254, # 3 dots / 300 dpi * 25.4mm/in
  "module_height": 10.0,   # The height of the barcode modules in mm
  "quiet_zone":     0.0,   # Distance on the left and on the right from the border to the first (last) barcode module in mm as float. Defaults to 6.5.
  "font_size":     18  ,   # Font size of the text under the barcode in pt as integer. Defaults to 10.
  "text_distance":  0.7,   # Distance between the barcode and the text under it in mm as float. Defaults to 5.0.
}

label_sizes = {
  "17x54": (566, 165), # printable area: 47.92mm x 13.97mm (566dots x 165dots in 300dpi = 135.84dots x 39.60dots in 72dpi)
  "17x87": (956, 165), # printable area: 80.94mm x 13.97mm (956dots x 165dots in 300dpi = 229.44dots x 39.60dots in 72dpi)
  "29x90": (991, 306), # printable area: 83.90mm x 25.91mm (991dots x 306dots in 300dpi = 237.84dots x 73.44dots in 72dpi)
}

pt300 = 0.24 # (pt300 = 0.24 pt72)

# Check print status
def checkPrintStatus():
    CHECK_MAX_SECONDS = 4
    data_received        = False
    print_successful     = False
    end_of_media_reached = False

    dev = os.open("/dev/usb/lp0", os.O_RDWR)
    try:
        start = time.time()
        while time.time() - start < CHECK_MAX_SECONDS:
            data = os.read(dev, 32)
            if len(data) == 32:
                data_received = True
                error_bytes = data[8], data[9]
                if (error_bytes[0] & 0x2) or (error_bytes[1] & 0x40):
                    end_of_media_reached = True
                    break
                if data[18] == 0x06 and data[19] == 0x00:
                    print_successful = True
                    break
    except KeyboardInterrupt:
        pass
    finally:
        os.close(dev)
    
    # Check for print success
    if print_successful == True:
        print("Print successful")
    elif end_of_media_reached == True:
        print(""                                                           )
        print("|!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!|")
        print("|---------------------------------------------------------|")
        print("|---------------------------------------------------------|")                 
        print("|                 CHANGE PRINTER LABEL ROLL!!!!           |")                    
        print("|---------------------------------------------------------|")
        print("|---------------------------------------------------------|")
        print("|!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!|")
        input("Press enter once done:")

def create_raster_file(label_size, in_file, out_file, cut=True):
    qlr = brother_ql.BrotherQLRaster(PRINTER_MODEL)
    brother_ql.create_label(qlr, in_file, label_size, cut=cut)
    with open(out_file, 'wb') as f:
        f.write(qlr.data)

def create_label_type1(label_size, barcode_value, line1=None, line2=None, line3=None):
    im = Image.new("L", label_sizes[label_size], 255)
    draw = ImageDraw.Draw(im)
    barcode_im = CODE128(barcode_value, writer=ImageWriter()).render(ptoptions)
    im.paste(barcode_im, (2, -5))
    x_off = barcode_im.size[0] + 30
    font = ImageFont.truetype(FONT, 44)
    width, height = font.getsize(line1)
    y_pos = 5
    draw.text((x_off, y_pos), line1, font=font, fill=0)
    draw.line((x_off, y_pos+height) + (x_off + width, y_pos+height), fill=0)
    font = ImageFont.truetype(FONT, 37)
    draw.text((x_off,  70), line2, font=font, fill=0)
    draw.text((x_off, 115), line3, font=font, fill=0)
    return im

def create_label_type2(label_size, text=None, font_size=11):
    im = Image.new("L", label_sizes[label_size], 255)
    draw = ImageDraw.Draw(im)
    font = ImageFont.truetype(FONT, int(font_size * 4.2))
    width, height = font.getsize(text)
    x_pos = int((im.size[0]-width)/2)
    y_pos = int((im.size[1]-height)/2-3)
    draw.text((x_pos, y_pos), text, font=font, fill=0)
    return im
    
def get_ble_color_for_serial_number(serial_number):
    return ["Problem", "PBM"]
    
def print_labels_for_ble_device(serial_number):    
    # 17*87mm label size
    label_size = "17x87"
    # Bar code value: BLE - Color - Serial
    barcode_value = "BLE-"+get_ble_color_for_serial_number(serial_number)[1]+"-"+str(serial_number).zfill(5)
    # Text: Mooltipass Mini / Color: XXX / Serial number: XXXX
    line1, line2, line3 = "Mooltipass Mini BLE", "Color: "+get_ble_color_for_serial_number(serial_number)[0], "Serial Number: "+str(serial_number).zfill(5)
    out_file = "label_number_1.bin"
    # Create label with content
    im = create_label_type1(label_size, barcode_value, line1, line2, line3)
    create_raster_file(label_size, im, out_file, cut=False)
    # Use cat to print label
    os.system("cat "+out_file+" > /dev/usb/lp0")
    
    # Check print status
    checkPrintStatus()
    
    # 17*87mm label size, text value: BLE - v1 - 8Mb - Color        
    label_size, text = "17x87", "BLE-v1-8Mb-"+get_ble_color_for_serial_number(serial_number)[1]
    out_file = "label_number_2.bin"
    # Create label with content
    im = create_label_type2(label_size, text, font_size=16)
    create_raster_file(label_size, im, out_file, cut=True)
    # Use cat to print label
    os.system("cat "+out_file+" > /dev/usb/lp0")
    
    # Check print status
    checkPrintStatus()

if __name__ == "__main__":
    print_labels_for_ble_device(32)

