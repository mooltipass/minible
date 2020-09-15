import random
import string

def get_random_string(length):
	letters = string.ascii_lowercase
	result_str = ''.join(random.choice(letters) for i in range(length))
	return result_str

nb_creds = input("Number of credentials (default 1000): ")

if nb_creds == "":
	nb_creds = 1000
else:
	nb_creds = int(nb_creds)

f = open("fakecreds.csv", "w")
for i in range(nb_creds):	
	f.write(get_random_string(1) + "service_" + get_random_string(5) + ",login_" + get_random_string(5) + ",pass_" + get_random_string(5)+"\n")
f.close()