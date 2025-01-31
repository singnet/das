# **NuNet Deployment Guide**  

The Device Management Service (DMS) allows a machine to join the decentralized NuNet network either as a compute provider—offering its resources to the network—or as a user leveraging the compute power of other machines for processing tasks.  

> For more details, visit the official documentation: [NuNet DMS](https://gitlab.com/nunet/device-management-service)  

## **Getting Started**
  
### **1. Initializing the Knowledge Base**

Some of the components require an existing knowledge base and a properly configured environment with Redis and MongoDB. You can set up this environment using the `das-cli`. For detailed instructions, refer to the [das-toolbox documentation](https://github.com/singnet/das-toolbox).

### **2. Configuring the Device Management Service and Deploying DAS Components to the NuNet Network**

To configure the Device Management Service (DMS) and deploy the DAS components to the NuNet network, run the following command:

```
make setup-nunet-dms
```

This script will:  
- Install the NuNet CLI  
- Create an organization on your local machine  
- Configure the user and DMS (if not already set up)  
- Onboard resources and deploy the components to the network  

### **3. Resetting NuNet Configuration**

If you need to reset the NuNet configuration and remove any previously made changes, you can use the following command:

```
make reset-nunet-dms
```

This command will:  
- Remove the local NuNet organization  
- Reset any user and DMS configurations  
- Clear onboarded resources  
