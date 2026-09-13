# Temperature Controller by Hysteresis using ESP32 with AWS IoT
This project demonstrates a temperature controller integrated with some AWS services, maintaining the temperature of a fridge during fermentation of craft beer.
## Project Overview
The beer production industry, the control of temperature is crucial at almost every stage, with critical importance during fermentation process, which is the focus of this project. This project addresses this challenge by implementing an **reverse-acting hysteresis controller** on an **ESP32**, and data can be accessed by any authenticated device connected to the internet. Communication is managed by AWS IoT Core, allowing for a seamless connection between the device and the cloud. This setup currently enables a real-time visualization of temperature data, as well as remote determination of the **setpoint** by the user. The prototype is a responsive and reliable control system where the hardware and software work in harmony to maintain stable temperature.

---

## Hardware Components
* **ESP32:** The brain of the system, controlling temperature of the fridge and handling Wi-Fi/MQTT communication to the Cloud.
* **DS18b20:** The primary waterproof sensor, measures temperature inside the bucket.
* **JQC-3FF-S-Z:** The relay used to control the fridge compressor.
> Note: This relay requires an external 5V power supply for reliable triggering. Therefore, an NPN Transistor was used, as shown in the diagram below. Connect relay's IN pin to GND.
![Alt text](/images/circuit.svg)
## Architecture Diagram
![Alt text](/images/iot_diagram.svg)

--- 

## Setup Guide 
### Create a thing
Create an IoT Policy
```bash
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": "iot:Connect",
      "Resource": "arn:aws:iot:REGION:ACCOUNT_ID:client/${iot:Connection.Thing.ThingName}"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Subscribe",
      "Resource": [
        "arn:aws:iot:REGION:ACCOUNT_ID:thing/esp32-ctemp-hy",
        "arn:aws:iot:REGION:ACCOUNT_ID:topicFilter/esp32/*",
        "arn:aws:iot:REGION:ACCOUNT_ID:topic/errors/${iot:Connection.Thing.ThingName}"
      ]
    },
    {
      "Effect": "Allow",
      "Action": [
        "iot:Publish",
        "iot:Receive",
        "iot:PublishRetain"
      ],
      "Resource": [
        "arn:aws:iot:REGION:ACCOUNT_ID:thing/esp32-ctemp-hy",
        "arn:aws:iot:REGION:ACCOUNT_ID:topic/esp32/*"
      ]
    }
  ]
}
```
Create it via CLI:
```bash
aws iot create-policy \
  --policy-name esp32Policy \
  --policy-document file://policy.json
```
Download the Amazon Root CA
```bash
curl https://www.amazontrust.com/repository/AmazonRootCA1.pem  > root-CA.crt
```
Create an IoT Thing
```bash
aws iot create-thing --thing-name "esp32-ctemp-hy"
```
Create Device Certificates
```bash
aws iot create-keys-and-certificate \
  --set-as-active \
  --certificate-pem-outfile device.pem.crt \
  --public-key-outfile "public.pem.key" \
  --private-key-outfile private.pem.key
```
Save the certificate ARN from the output, to attach this to your thing.
```bash
aws iot attach-thing-principal \
    --thing-name "esp32-ctemp-hy" \
    --principal YOUR_CERT_ARN
```
Attach Policy to Certificate
```bash
aws iot attach-policy \
  --policy-name esp32Policy \
  --target YOUR_CERT_ARN
```
Get Your AWS IoT Endpoint
```bash
aws iot describe-endpoint --endpoint-type iot:Data-ATS
```
### DynamoDB Setup
Create a DynamoDB table:
- Table name: ctemp-hy
- Partition key: timestamp (String)
- Sort key: device_id (Number)
```bash
aws dynamodb create-table \
    --table-name ctemp-hy \
    --attribute-definitions \
        AttributeName=timestamp,AttributeType=S \
        AttributeName=device_id,AttributeType=N \
    --key-schema AttributeName=timestamp,KeyType=HASH AttributeName=device_id,KeyType=RANGE \
    --billing-mode PAY_PER_REQUEST \
    --table-class STANDARD

```
### Lambda Function
Create a role attaching a managed dynamodb policy, to allow data pipeline. After that, create a rule for IoT core.
#### IAM Permissions for Lambda
Attach this policy to the Lambda execution role:
```bash
{
  "Effect": "Allow",
  "Action": "dynamodb:PutItem",
  "Resource": "arn:aws:dynamodb:REGION:ACCOUNT_ID:table/ctemp-hy"
}
```
#### AWS IoT Rule
```bash
{
  "sql": "SELECT * FROM 'esp32/pub'",
  "ruleDisabled": false,
  "awsIotSqlVersion": "2016-03-23",
  "actions": [
          {
                  "lambda": {
                          "functionArn": "arn:aws:lambda:us-east-1:652259>
                  }
          }
  ]
}
```