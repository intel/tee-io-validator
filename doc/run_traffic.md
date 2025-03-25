# Run traffic in IDE Stream
## Instal FIO Tool
[FIO](https://github.com/axboe/fio) is an open source tool that will spawn a number of threads or processes doing a particular type of I/O action as specified by the user.
Its [binary packages](https://github.com/axboe/fio#binary-packages) have been part of the linux distribution repositories.
Here are some examples to install FIO.
```
## For Ubuntu
sudo apt-get -y install fio

## For CentOS
sudo yum -y install fio
```
## Find the TEEIO Device information
Follow below steps to prepare the TEEIO Device information which will be used in FIO command options.
**Step 1**: Use lspci to check the device (For example device's BDF is da:00.0)
```
# lspci -s da:00.0 -v
da:00.0 Non-Volatile memory controller: SomeVendor's Device [NVM Express])
        Subsystem: SomeVendor's Device aa99
        Physical Slot: 19
        Flags: bus master, fast devsel, latency 0, IRQ 16, NUMA node 0
        ... ...
        Kernel driver in use: nvme
        Kernel modules: nvme
```
In the output of lspci, check ```Kernel driver in use: nvme```

**Step 2**: Check the actual device
```
# ls /sys/bus/pci/devices/0000\:da\:00.0/nvme
nvme5
```

**Step 3**: Make sure /dev/nvme5 exists
```
# ls /dev/nvme5
/dev/nvme5
```

## Prepare the FIO command
Run below command to test I/O traffic with FIO.
```
FILENAME=/dev/nvme5

sudo fio --filename=${FILENAME} --rw=rw \
    --direct=1 --bs=4K --ioengine=libaio \
    --runtime=300 --numjobs=1 --time_based \
    --group_reporting --name=seq_readwrite \
    --iodepth=16 \
    --output=result_seqwrite_IDE.txt
```
Note: If tester want to test 2 Devices with one FIO command, follow [Find the TEEIO Device information](#find-the-teeio-device-information) to find both devices' information, for example, ```/dev/nvme5``` and ```/dev/nvme6```, then set ```FILENAME=/dev/nvme5:/dev/nvme6```.

