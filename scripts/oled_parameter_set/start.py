from mooltipass_hid_device import *
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

class OledControlApp(tk.Tk):

	def value_changed_callback(self):
		contrast_val = int(self.contrast_current_var.get())
		vcomh_val = int(self.vcomh_var.get())
		vsegm_val = int(self.vsegm_var.get())
		precharge_per_val = int(self.precharge_period_var.get())
		discharge_per_val = int(self.discharge_period_var.get())
		discharge_val = int(self.discharge_level_var.get())
		
		# If vcomh doesn't change, send 0xFF so the MCU doesn't use display on & off
		if self.last_vcomh == vcomh_val:
			vcomh_val = 0xFF
		else:
			self.last_vcomh = vcomh_val
		
		# Log output
		self.log_output_text.insert("end", "cur " + hex(contrast_val) + " vcomh " + hex(vcomh_val) + " vsegm " + hex(vsegm_val) + " pre_per " + hex(precharge_per_val) + " dis_per " + hex(discharge_per_val) + " dis " + hex(discharge_val) + "\r\n")
		self.log_output_text.see(tk.END)
		
		# Send packet
		self.mooltipass_device.sendOledParameters(contrast_val, vcomh_val, vsegm_val, precharge_per_val, discharge_per_val, discharge_val)

	def __init__(self, *args, **kwargs):
		tk.Tk.__init__(self, *args, **kwargs)

		self.title_font = tkfont.Font(family='Helvetica', size=18, weight="bold")
		self.configure(background='LightSteelBlue2')
		self.title("OLED Settings Test GUI")
		self.resizable(0,0)
		self.focus_set()
		self.bind("<Escape>", lambda e: e.widget.quit())
		
		# Contrast current
		contrast_current_label = tk.Label(self, text="Contrast current :", background="LightSteelBlue2", font=tkfont.Font(family='Helvetica', size=12), anchor="e")
		contrast_current_label.grid(sticky="e", row=0, column=0, pady=(18,2), padx = 5)
		self.contrast_current_var = tk.DoubleVar()
		contrast_current_scale = tk.Scale(self, variable=self.contrast_current_var, command=lambda x:[self.value_changed_callback()], from_=0, to=255, length=300, orient=tk.HORIZONTAL, relief="flat", bd=2, bg="LightSteelBlue2", highlightbackground="LightSteelBlue2")  
		contrast_current_scale.grid(row=0, column=1, pady=2, padx = 5)
		self.contrast_current_var.set(0x90)
		
		# VCOMH
		vcomh_label = tk.Label(self, text="VCOMH level :", background="LightSteelBlue2", font=tkfont.Font(family='Helvetica', size=12), anchor="e")
		vcomh_label.grid(sticky="e", row=1, column=0, pady=(18,2), padx = 5)
		self.vcomh_var = tk.DoubleVar()
		vcomh_scale = tk.Scale(self, variable=self.vcomh_var, command=lambda x:[self.value_changed_callback()], from_=0, to=255, length=300, orient=tk.HORIZONTAL, relief="flat", bd=2, bg="LightSteelBlue2", highlightbackground="LightSteelBlue2")  
		vcomh_scale.grid(row=1, column=1, pady=2, padx = 5)
		self.vcomh_var.set(0x30)
		self.last_vcomh = 0x30
		
		# VSEGM
		vsegm_label = tk.Label(self, text="VSEGM level :", background="LightSteelBlue2", font=tkfont.Font(family='Helvetica', size=12), anchor="e")
		vsegm_label.grid(sticky="e", row=2, column=0, pady=(18,2), padx = 5)
		self.vsegm_var = tk.DoubleVar()
		vsegm_scale = tk.Scale(self, variable=self.vsegm_var, command=lambda x:[self.value_changed_callback()], from_=0, to=255, length=300, orient=tk.HORIZONTAL, relief="flat", bd=2, bg="LightSteelBlue2", highlightbackground="LightSteelBlue2")  
		vsegm_scale.grid(row=2, column=1, pady=2, padx = 5)
		self.vsegm_var.set(0x00)
		
		# Precharge period
		precharge_period_label = tk.Label(self, text="Precharge period :", background="LightSteelBlue2", font=tkfont.Font(family='Helvetica', size=12), anchor="e")
		precharge_period_label.grid(sticky="e", row=3, column=0, pady=(18,2), padx = 5)
		self.precharge_period_var = tk.DoubleVar()
		precharge_period_scale = tk.Scale(self, variable=self.precharge_period_var, command=lambda x:[self.value_changed_callback()], from_=0, to=8, sliderlength=100, length=300, orient=tk.HORIZONTAL, relief="flat", bd=2, bg="LightSteelBlue2", highlightbackground="LightSteelBlue2")  
		precharge_period_scale.grid(row=3, column=1, pady=2, padx = 5)
		self.precharge_period_var.set(6)
		
		# Discharge period
		dishcharge_period_label = tk.Label(self, text="Discharge period :", background="LightSteelBlue2", font=tkfont.Font(family='Helvetica', size=12), anchor="e")
		dishcharge_period_label.grid(sticky="e", row=4, column=0, pady=(18,2), padx = 5)
		self.discharge_period_var = tk.DoubleVar()
		discharge_period_scale = tk.Scale(self, variable=self.discharge_period_var, command=lambda x:[self.value_changed_callback()], from_=0, to=8, sliderlength=100, length=300, orient=tk.HORIZONTAL, relief="flat", bd=2, bg="LightSteelBlue2", highlightbackground="LightSteelBlue2")  
		discharge_period_scale.grid(row=4, column=1, pady=2, padx = 5)
		self.discharge_period_var.set(7)
		
		# Discharge level
		discharge_level_label = tk.Label(self, text="Discharge level :", background="LightSteelBlue2", font=tkfont.Font(family='Helvetica', size=12), anchor="e")
		discharge_level_label.grid(sticky="e", row=5, column=0, pady=(18,2), padx = 5)
		self.discharge_level_var = tk.DoubleVar()
		discharge_level_scale = tk.Scale(self, variable=self.discharge_level_var, command=lambda x:[self.value_changed_callback()], from_=0, to=3, sliderlength=100, length=300, orient=tk.HORIZONTAL, relief="flat", bd=2, bg="LightSteelBlue2", highlightbackground="LightSteelBlue2")  
		discharge_level_scale.grid(row=5, column=1, pady=2, padx = 5)
		self.discharge_level_var.set(1)
		
		# Log output
		self.log_output_text = tk.Text(self, width=70, height=8, wrap=tk.WORD)
		self.log_output_text.grid(row=6, column=0, columnspan=2, pady=20, padx = 20)
		
		# Device connection
		self.mooltipass_device = mooltipass_hid_device()	
		
		# Connect to device
		if self.mooltipass_device.connect(True) == False:
			sys.exit(0)
			#pass
		
		# Debug
		self.log_output_text.insert("end", "Connected to device\r\n")
		self.log_output_text.see(tk.END)


if __name__ == "__main__":
	app = OledControlApp()
	app.mainloop()