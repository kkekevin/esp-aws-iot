import json
import boto3
import os
from datetime import datetime

dynamodb = boto3.resource('dynamodb')

# Grab the table name from an environment variable (best practice)
TABLE_NAME = os.environ.get('DYNAMODB_TABLE', 'ctemp-hy')
table = dynamodb.Table(TABLE_NAME)

def lambda_handler(event, context):
    try:
        # Log the incoming event from IoT Core
        print("Received IoT event: " + json.dumps(event))
        
        # If your device doesn't include a timestamp, generate one here
        #timestamp = event.get('timestamp', str(int(datetime.utcnow().timestamp())))
        device_id = event.get('device_id')
        
        if not device_id:
            raise ValueError("Missing 'device_id' in MQTT payload.")
            
        db_item = {
            'device_id': device_id,                       # Assumed Partition Key 
            'timestamp': event.get('timestamp'),          # Assumed Sort Key (if applicable)
            'temperature': event.get('temperature'),
            'status': event.get('msg', 'OK'),
        }
        
        # Remove None values so DynamoDB doesn't throw errors
        db_item = {k: v for k, v in db_item.items() if v is not None}
        
        # 4. Write to DynamoDB
        response = table.put_item(Item=db_item)
        
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
