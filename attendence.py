import serial
import csv
import time
import os
import gspread
from oauth2client.service_account import ServiceAccountCredentials
from datetime import datetime

# --- CONFIGURATION ---
COM_PORT = 'COM5'       # <--- CHECK THIS
BAUD_RATE = 115200      
USER_DB = 'users.csv'   
JSON_KEY_FILE = 'credentials.json' 
GOOGLE_SHEET_NAME = 'Attendance'   

# --- PRE-FLIGHT CHECKS ---
if not os.path.exists(JSON_KEY_FILE):
    print(f"❌ Error: {JSON_KEY_FILE} not found.")
    exit()
if not os.path.exists(USER_DB):
    print(f"❌ Error: {USER_DB} not found.")
    exit()

# --- GOOGLE SHEETS SETUP ---
try:
    scope = ["https://spreadsheets.google.com/feeds", "https://www.googleapis.com/auth/drive"]
    creds = ServiceAccountCredentials.from_json_keyfile_name(JSON_KEY_FILE, scope)
    client = gspread.authorize(creds)
    sheet = client.open(GOOGLE_SHEET_NAME).sheet1
    print("✅ Connected to Google Sheets!")
except Exception as e:
    print(f"❌ Google Error: {e}")
    exit()

# --- LOAD LOCAL DATABASE ---
user_db = {}
try:
    with open(USER_DB, mode='r') as f:
        reader = csv.reader(f)
        next(reader, None) 
        for row in reader:
            if len(row) >= 2:
                user_db[row[0].strip()] = row[1].strip()
    print(f"✅ Database Loaded ({len(user_db)} users).")
except Exception as e:
    print(f"❌ CSV Error: {e}")
    exit()

# --- MAIN LOOP ---
try:
    ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=1)
    time.sleep(2)
    print(f"✅ Connected to {COM_PORT}. Ready to scan...")
    print("------------------------------------------------")

    while True:
        if ser.in_waiting > 0:
            uid = ser.readline().decode('utf-8', errors='ignore').strip()
            
            if uid and len(uid) > 4:
                # Check Database
                if uid in user_db:
                    name = user_db[uid]
                    
                    # 1. Send Name to LCD IMMEDIATELY (So it feels fast)
                    ser.write(f"{name}\n".encode('utf-8'))
                    
                    # 2. Log to Cloud (Every single time)
                    now = datetime.now()
                    date_str = now.strftime("%Y-%m-%d")
                    time_str = now.strftime("%H:%M:%S")
                    
                    try:
                        sheet.append_row([uid, name, date_str, time_str, "Present"])
                        print(f"☁️  Logged: {name} at {time_str}")
                    except Exception as e:
                        print(f"❌ Upload Failed: {e}")
                    
                else:
                    print(f"❌ Unknown Card: {uid}")
                    ser.write(b"DENIED\n")

except KeyboardInterrupt:
    print("\nStopping...")
except serial.SerialException:
    print(f"❌ Connection Error: Is Arduino Serial Monitor open?")