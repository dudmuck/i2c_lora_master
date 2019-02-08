# LoRa I2C master
Using [raspberry Pi](https://en.wikipedia.org/wiki/Raspberry_Pi) over I2C to LoRa radio.

Operates with slave device [i2c_lora_slave](https://os.mbed.com/users/dudmuck/code/i2c_lora_slave/)


# build instructions
for raspberry pi, initial installation:
* if ``/dev/i2c-1`` doesnt exist, use ``raspi-config`` to enable it, or edit ``/boot/config.txt`` to uncomment ``dtparam=i2c_arm=on``
* [install wiringPi](http://wiringpi.com/download-and-install/) library
* ``sudo apt install libi2c-dev swig``

Build:
* ``mkdir build``
* ``cd build``
* ``cmake ..`` 
* ``make``

# wiring connections
see [lib_i2c_slave_block](https://os.mbed.com/users/dudmuck/code/lib_i2c_slave_block/) for wiring connections to slave.

4 pins required: SDA, SCL, interrupt and ground.

# testing
Utility ``testI2C`` gives command line access to radio.  Run with no argument to see available commands.
```
```
