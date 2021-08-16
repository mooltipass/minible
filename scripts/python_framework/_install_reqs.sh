sudo apt update
sudo apt-get install -y python3-pip libgmp-dev libmpfr-dev libmpc-dev git libatlas-base-dev libopenjp2-7 libtiff5 bluetooth libbluetooth-dev < /dev/null
sudo python3 -m pip install -r requirements.txt
sudo python3 -m pip install pybluez