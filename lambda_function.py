import json
import boto3
import os
from datetime import datetime

dynamodb = boto3.resource('dynamodb')
sns_client = boto3.client('sns')

# Grab the table name from an environment variable (best practice)
TABLE_NAME = os.environ.get('DYNAMODB_TABLE', 'ctemp-hy')
table = dynamodb.Table(TABLE_NAME)

def lambda_handler(event, context):
    try:
        # Log the incoming event from IoT Core
        print("Received IoT event: " + json.dumps(event))
        
        # Retrieve json data from mqtt
        device_id = event.get('device_id')
        timestamp = event.get('timestamp')
        temperature = event.get('temperature')
        # If your device doesn't include a timestamp
        # timestamp = event.get('timestamp', str(int(datetime.utcnow().timestamp())))
        phone_number = event.get('phone_number')

        if not device_id:
            raise ValueError("Missing 'device_id' in MQTT payload.")
            
        db_item = {
            'device_id': device_id,                       # Assumed Partition Key 
            'timestamp': timestamp,                       # Assumed Sort Key (if applicable)
            'temperature': temperature,
            'status': event.get('msg', 'SENSOR DISCONNECTED')
        }
        
        # Remove None values so DynamoDB doesn't throw errors
        db_item = {k: v for k, v in db_item.items() if v is not None}
        
        # Write to DynamoDB
        response = table.put_item(Item=db_item)

        # check critical notifications when phone number included
        if phone_number:
            message = f"Temp sensor of ESP{device_id} is unplugged, the previous temperature was {temperature}"
            response = sns_client.publish(
                PhoneNumber=phone_number,
                Message=message,
                MessageAttributes={
                    'AWS.SNS.SMS.SMSType': {
                        'DataType': 'String',
                        'StringValue': 'Promotional' # or 'Transactional'
                    }
                }
            )
            return {
                'statusCode': 200,
                'body': json.dumps({'message': 'SMS sent successfully and logged to DDB', 'messageId': response['MessageId']})
            }
        
        return {
            'statusCode': 200,
            'body': json.dumps('Data successfully written to DynamoDB')
        }
        
    except Exception as e:
        print(f"Error processing IoT data: {str(e)}")
        return {
            'statusCode': 500,
            'body': json.dumps(f"Failed to store data: {str(e)}")
        }
