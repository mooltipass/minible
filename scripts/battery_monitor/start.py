from __future__ import print_function
from mooltipass_hid_device import *
from time import gmtime, strftime, localtime
from time import sleep
try:
	import tkFont as tkfont
	import Tkinter as tk
	import ttk
except ImportError:
	from tkinter import font as tkfont
	import tkinter.ttk as ttk
	import tkinter as tk
import cmath
import math
import time
import sys
		
def close_app(controller):
	controller.destroy()

class BatteryMonitorApp(tk.Tk):

	def monitor_button_action(self):
		if self.monitoring_bool:
			self.monitoring_bool = False
			self.monitor_button.config(text="Start Battery Monitoring")
		else:
			self.monitoring_bool = True
			self.monitor_button.config(text="Stop Battery Monitoring")
			self.monitoring_routine()
			
	def monitoring_routine(self):
		# Continue Monitoring
		if self.monitoring_bool:
			self.action_button(False,False,False,False,0,False)
			self.after(2000, self.monitoring_routine)		

	def action_button(self, start_charging, stop_charging, switch_to_usb_screen_power, switch_to_battery_screen_power, force_charge_voltage, stop_force_charge):
		# Send action to device
		packet = self.mooltipass_device.getBatteryStatus(start_charging, stop_charging, switch_to_usb_screen_power, switch_to_battery_screen_power, force_charge_voltage, stop_force_charge)
		
		if packet["cmd"] != CMD_ID_RETRY and packet["cmd"] != CMD_ID_GET_STATUS:
			power_source = struct.unpack('I', packet["data"][0:4])[0]
			charging_bool = struct.unpack('I', packet["data"][4:8])[0]
			main_adc_val = struct.unpack('H', packet["data"][8:10])[0]
			aux_charge_status = struct.unpack('H', packet["data"][10:12])[0]
			aux_battery_voltage = struct.unpack('H', packet["data"][12:14])[0]
			aux_charge_current = struct.unpack('H', packet["data"][14:16])[0]
			aux_stepdown_voltage = struct.unpack('H', packet["data"][16:18])[0]
			aux_dac_data_reg = struct.unpack('H', packet["data"][18:20])[0]
			
			if power_source == 0:
				self.power_source_value.config(text='USB')
			else:
				self.power_source_value.config(text='Battery')			
			if charging_bool == 0:
				self.charging_state_value.config(text='Not Charging')
			else:
				self.charging_state_value.config(text='Charging')
			self.main_adc_voltage_value.config(text=str((main_adc_val*199)>>9) + "mV")
			self.aux_adc_voltage_value.config(text=str(round(aux_battery_voltage*0.5445)) + "mV")
			self.aux_charge_status_value.config(text=str(aux_charge_status))
			self.aux_charge_current_value.config(text=str(int(aux_charge_current*0.5445))+"mA")
			self.aux_stepdown_voltage_value.config(text=str(aux_stepdown_voltage))
			self.aux_data_register_value.config(text=str(aux_dac_data_reg))
		
			# Log output
			self.log_output_text.insert("end", strftime("%Y-%m-%d %H:%M:%S", localtime()) + ": " + str(power_source) + "/" + str(charging_bool) + "/" + str(aux_charge_status) + "/" + str(main_adc_val) + "/" + str(aux_battery_voltage) + "/" + str(aux_charge_current) + "/" + str(aux_stepdown_voltage) + "/" + str(aux_dac_data_reg)  + "\r\n")
			self.log_output_text.see(tk.END)
			
			# Create log file if needed
			if self.log_file_opened == False:
				self.log_file_fd = open(str(strftime("%Y-%m-%d_%H-%M-%S", localtime())) + "_voltage_log.txt", "wt+")
				self.log_file_opened = True
				
			# Log data
			self.log_file_fd.write(str(self.log_counter) + ",")
			self.log_file_fd.write(strftime("%Y-%m-%d_%H:%M:%S", localtime()) + ",")
			self.log_file_fd.write(str((main_adc_val*199)>>9) + ",")
			self.log_file_fd.write(str(aux_battery_voltage) + ",")
			self.log_file_fd.write(str(round(aux_battery_voltage*0.5445)) + ",")
			self.log_file_fd.write(str(aux_charge_current) + ",")
			self.log_file_fd.write(str(int(aux_charge_current*0.5445)) + ",")
			self.log_file_fd.write(str(aux_charge_status) + ",")
			self.log_file_fd.write(str(aux_stepdown_voltage) + ",")
			self.log_file_fd.write(str(aux_dac_data_reg))
			self.log_file_fd.write("\r")
			self.log_file_fd.flush()
			self.log_counter+=1
			
			# Force UI update
			self.update_idletasks()
			self.update()

	def __init__(self, *args, **kwargs):
		tk.Tk.__init__(self, *args, **kwargs)

		self.title_font = tkfont.Font(family='Helvetica', size=18, weight="bold")
		self.configure(background='LightSteelBlue2')
		self.title("Battery Monitoring Tool")
		self.resizable(0,0)
		self.focus_set()
		self.bind("<Escape>", lambda e: e.widget.quit())
		
		# Vars
		self.monitoring_bool = False
		self.log_file_opened = False
		self.log_counter = 0
		
		# Power Source
		self.power_source_label = tk.Label(self, text="Power Source :", background="LightSteelBlue2", font=tkfont.Font(family='Helvetica', size=12), anchor="e")
		self.power_source_label.grid(sticky="e", row=0, column=0, pady=(5,2), padx=5)
		self.power_source_value = tk.Label(self, text="Battery", background="LightSteelBlue2", font=tkfont.Font(family='Helvetica', size=12, weight=tkfont.BOLD))
		self.power_source_value.grid(row=0, column=1, pady=(5,2), padx=5)
		
		# Charging State
		self.charging_state_label = tk.Label(self, text="Charging State :", background="LightSteelBlue2", font=tkfont.Font(family='Helvetica', size=12), anchor="e")
		self.charging_state_label.grid(sticky="e", row=0, column=2, pady=(5,2), padx=5)
		self.charging_state_value = tk.Label(self, text="Charging", background="LightSteelBlue2", font=tkfont.Font(family='Helvetica', size=12, weight=tkfont.BOLD))
		self.charging_state_value.grid(row=0, column=3, pady=(5,2), padx=5)
		
		# Main ADC Value
		self.main_adc_voltage_label = tk.Label(self, text="Main ADC Voltage :", background="LightSteelBlue2", font=tkfont.Font(family='Helvetica', size=12), anchor="e")
		self.main_adc_voltage_label.grid(sticky="e", row=1, column=0, pady=(0,2), padx=5)
		self.main_adc_voltage_value = tk.Label(self, text="1000mV", background="LightSteelBlue2", font=tkfont.Font(family='Helvetica', size=12, weight=tkfont.BOLD))
		self.main_adc_voltage_value.grid(row=1, column=1, pady=(0,2), padx=5)
		
		# Aux ADC Value
		self.aux_adc_voltage_label = tk.Label(self, text="Aux ADC Voltage :", background="LightSteelBlue2", font=tkfont.Font(family='Helvetica', size=12), anchor="e")
		self.aux_adc_voltage_label.grid(sticky="e", row=1, column=2, pady=(0,2), padx=5)
		self.aux_adc_voltage_value = tk.Label(self, text="1000mV", background="LightSteelBlue2", font=tkfont.Font(family='Helvetica', size=12, weight=tkfont.BOLD))
		self.aux_adc_voltage_value.grid(row=1, column=3, pady=(0,2), padx=5)
		
		# Aux Charge Status
		self.aux_charge_status_label = tk.Label(self, text="Aux Charge Status :", background="LightSteelBlue2", font=tkfont.Font(family='Helvetica', size=12), anchor="e")
		self.aux_charge_status_label.grid(sticky="e", row=2, column=0, pady=(0,2), padx=5)
		self.aux_charge_status_value = tk.Label(self, text="2", background="LightSteelBlue2", font=tkfont.Font(family='Helvetica', size=12, weight=tkfont.BOLD))
		self.aux_charge_status_value.grid(row=2, column=1, pady=(0,2), padx=5)
		
		# Aux Charge Current
		self.aux_charge_current_label = tk.Label(self, text="Aux Charge Current :", background="LightSteelBlue2", font=tkfont.Font(family='Helvetica', size=12), anchor="e")
		self.aux_charge_current_label.grid(sticky="e", row=2, column=2, pady=(0,2), padx=5)
		self.aux_charge_current_value = tk.Label(self, text="2", background="LightSteelBlue2", font=tkfont.Font(family='Helvetica', size=12, weight=tkfont.BOLD))
		self.aux_charge_current_value.grid(row=2, column=3, pady=(0,2), padx=5)
		
		# Aux Stepdown Voltage
		self.aux_stepdown_voltage_label = tk.Label(self, text="Aux Stepdown Voltage :", background="LightSteelBlue2", font=tkfont.Font(family='Helvetica', size=12), anchor="e")
		self.aux_stepdown_voltage_label.grid(sticky="e", row=3, column=0, pady=(0,2), padx=5)
		self.aux_stepdown_voltage_value = tk.Label(self, text="2", background="LightSteelBlue2", font=tkfont.Font(family='Helvetica', size=12, weight=tkfont.BOLD))
		self.aux_stepdown_voltage_value.grid(row=3, column=1, pady=(0,2), padx=5)
		
		# Aux DAC data register value
		self.aux_data_register_label = tk.Label(self, text="Aux DAC DATA :", background="LightSteelBlue2", font=tkfont.Font(family='Helvetica', size=12), anchor="e")
		self.aux_data_register_label.grid(sticky="e", row=3, column=2, pady=(0,2), padx=5)
		self.aux_data_register_value = tk.Label(self, text="2", background="LightSteelBlue2", font=tkfont.Font(family='Helvetica', size=12, weight=tkfont.BOLD))
		self.aux_data_register_value.grid(row=3, column=3, pady=(0,2), padx=5)
		
		# Action Buttons
		self.platform_connect_button = tk.Button(self, text="Start Charge", font=tkfont.Font(family='Helvetica', size=9), width="18", command=lambda:[self.action_button(True, False, False, False, 0, False)])
		self.platform_connect_button.grid(row=4, column=0, pady=(5,2), padx=5)
		self.platform_connect_button = tk.Button(self, text="Stop Charge", font=tkfont.Font(family='Helvetica', size=9), width="18", command=lambda:[self.action_button(False, True, False, False, 0, False)])
		self.platform_connect_button.grid(row=4, column=1, pady=(5,2), padx=5)
		self.platform_connect_button = tk.Button(self, text="Screen Power: USB", font=tkfont.Font(family='Helvetica', size=9), width="18", command=lambda:[self.action_button(False, False, True, False, 0, False)])
		self.platform_connect_button.grid(row=4, column=2, pady=(5,2), padx=5)
		self.platform_connect_button = tk.Button(self, text="Screen Power: Battery", font=tkfont.Font(family='Helvetica', size=9), width="18", command=lambda:[self.action_button(False, False, False, True, 0, False)])
		self.platform_connect_button.grid(row=4, column=3, pady=(5,2), padx=5)
		
		# Log output
		self.log_output_text = tk.Text(self, width=80, height=8, wrap=tk.WORD)
		self.log_output_text.grid(row=5, column=0, columnspan=4, pady=(15,2), padx = 20)
		
		# Monitor Button
		self.empty_label = tk.Label(self, text="", background="LightSteelBlue2", font=tkfont.Font(family='Helvetica', size=12), anchor="e")
		self.empty_label.grid(sticky="e", row=6, column=0, pady=(5,2), padx=5)
		self.monitor_button = tk.Button(self, text="Start Battery Monitoring", font=tkfont.Font(family='Helvetica', size=9), width="40", command=lambda:[self.monitor_button_action()])
		self.monitor_button.grid(row=6, column=1, columnspan=2, pady=(5,2), padx=5)
		
		# Force Charge Voltage Row
		self.force_charge_voltage_label = tk.Label(self, text="Force Charge Voltage :", background="LightSteelBlue2", font=tkfont.Font(family='Helvetica', size=12), anchor="e")
		self.force_charge_voltage_label.grid(sticky="e", row=7, column=0, pady=(25,15), padx=5)
		self.frame_mv = tk.Frame(self, background="LightSteelBlue2")
		self.force_charge_voltage_text_var = tk.IntVar()
		self.force_charge_voltage_text = tk.Entry(self.frame_mv, width=10, justify='center', textvariable=self.force_charge_voltage_text_var)
		self.force_charge_voltage_text.pack(side=tk.LEFT,pady=(0,0),padx=(0,0))
		self.force_charge_voltage_label_mv = tk.Label(self.frame_mv, text=" mV", background="LightSteelBlue2", font=tkfont.Font(family='Helvetica', size=12))
		self.force_charge_voltage_label_mv.pack(side=tk.LEFT,pady=(0,0),padx=(0,0))
		self.frame_mv.grid(row=7, column=1, pady=(25,15), padx=5)
		self.platform_connect_button = tk.Button(self, text="Force Charge Voltage", font=tkfont.Font(family='Helvetica', size=9), width="18", command=lambda:[self.action_button(False, False, False, False, self.force_charge_voltage_text_var.get(), False)])
		self.platform_connect_button.grid(row=7, column=2, pady=(25,15), padx=5)
		self.platform_connect_button = tk.Button(self, text="Stop Force Charge", font=tkfont.Font(family='Helvetica', size=9), width="18", command=lambda:[self.action_button(False, False, False, False, 0, True)])
		self.platform_connect_button.grid(row=7, column=3, pady=(25,15), padx=5)
		
		# Device connection
		self.mooltipass_device = mooltipass_hid_device()	
		
		# Connect to device
		if self.mooltipass_device.connect(True) == False:
			sys.exit(0)
		
		# Debug
		self.log_output_text.insert("end", "Connected to device\r\n")
		self.log_output_text.see(tk.END)
		
		# Force UI update
		self.update_idletasks()
		self.update()
		
		# Initial value fetch
		self.after(500, self.action_button(False,False,False,False,0,False))


if __name__ == "__main__":
	app = BatteryMonitorApp()
	app.mainloop()
