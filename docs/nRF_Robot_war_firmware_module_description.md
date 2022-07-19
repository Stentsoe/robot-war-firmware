# nRF Robot war firmware modules


## Gateway nRF9160
AWS IoT device. Has two main tasks:
- Receive desired state from the cloud and translate it into robot operations.
- Track and report robot state and operation status.
  
### Modem
- Initiate and manage connection to LTE network.

### Cloud
- Notify gateway about state changes in the cloud.
- Notify cloud about state changes in the gateway.
- Initiate and manage AWS IoT connection.

### Gateway
- Track state of connected robots
- Trigger operations on robots
- Forward status of operations to the cloud
- Cloud payload formatting

### UART
- Ensure healthy state of gateway nRF52840
- Forward operations to gateway nRF52840
- Report messages from gateway nRF52840

## Gateway nRF52840
Acts as BTLE radio for nRF9160.

### UART
- Forward operations to gateway nRF52840 mesh.
- Report messages from mesh to gateway nRF9160.

### Mesh
- Define robot client model
- Initiate radio communication
- Forward operations to robot
- Report status messages from robot
- Listen for new robots

## Robot nRF52840
Controls and individual robot. Handles mesh communication and execution of robot operations.
### Mesh
- Define robot server model
- Initiate radio communication
- Report operations to be executed

### Motor
- Calculate required motor actions
- Actuate motors

# Interface descriptions

## Modem &rarr; Cloud
### Connected
- Notify cloud module that the device is connected to the LTE network.
### Disconnected
- Notify cloud module that the device is disconnected to the LTE network.

## AWS IoT &rarr; Cloud
