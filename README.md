# tc-nn-camera-app 
 
The test application (**tcnncameraapp**) is a sample application that allows you to test the camera used in the **TCC750x**. It operates based on the **V4L2 (Video for Linux 2)** interface and enables real-time camera preview. Additionally, the application supports simultaneous monitoring of multiple cameras.
 
## Supported Camera Configurations
The following are the maximum numbers of cameras that can be monitored simultaneously using this application:
 
- **IMX290**: 3× FHD ISP DVRS
- **AR0820**: 1× QHD ISPLESS
- **AR0239**: 2× FHD RGB-IR ISPLESS
 
---
 
## Application Options
The options provided by `tcnncameraapp` are shown in the following table:
 
| **Name**                | **Option** | **Range**         | **Default**    | **Example**         | **Description**                                                                                                                |
|-------------------------|------------|-------------------|----------------|---------------------|--------------------------------------------------------------------------------------------------------------------------------|
| **Usage info**          | `-?`       |                   |                | `-?`                | Show the usage information                                                                                                     |
| **Input Path**          | `-p`       |                   | `/dev/video0`  | `-p /dev/video0`    | Mode to specify the input path                                                                                                 |
| **Output Path**         | `-P`       |                   | `/dev/overlay` | `-P /dev/overlay`   | Mode to specify the output path                                                                                                |
| **Input Width**         | `-w`       | Up to 2560        | 1920           | `-w 2560`           | Mode to specify the width of the input                                                                                         |
| **Input Height**        | `-h`       | Up to 1440        | 1080           | `-h 1080`           | Mode to specify the height of the input                                                                                        |
| **Output Width**        | `-W`       | Up to 1920        | 1920           | `-W 1920`           | Mode to specify the width of the output                                                                                        |
| **Output Height**       | `-H`       | Up to 720         | 720            | `-H 720`            | Mode to specify the height of the output                                                                                       |
| **Output Position X**   | `-X`       | Up to 1920        | 0              | `-X 960`            | Mode to specify the X-coordinate of the output.<br>The sum of the output width and the output position X must not exceed 1920. |
| **Output Position Y**   | `-Y`       | Up to 720         | 0              | `-Y 360`            | Mode to specify the Y-coordinate of the output.<br>The sum of the output height and the output position Y must not exceed 720. |
 
---
 
This table summarizes the key options available for configuring the `tcnncameraapp` and their respective usage.