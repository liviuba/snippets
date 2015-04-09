package liviu.examples.spamvladucu;

import android.telephony.SmsManager;
import android.app.Service;
import android.widget.Toast;
import android.os.IBinder;
import android.content.Intent;

public class SpamVladucuService extends Service{
	@Override
	public int onStartCommand( Intent intent, int flags, int startId ){
		SmsManager smsManager = SmsManager.getDefault();
		String phoneNumber = intent.getStringExtra("phoneNumber");
		String smsMessage = intent.getStringExtra("smsMessage");
		/*
		long seconds = 0;
		long minutes = Integer.parseInt( intent.getStringExtra("howOften") );
		long hours = 0;
		long spamCount = Integer.parseInt( intent.getStringExtra("howMany") );
		*/
		try{
			smsManager.sendTextMessage( phoneNumber, null, smsMessage, null, null);
			Toast.makeText( getApplicationContext(), "SMS Sent!", Toast.LENGTH_LONG ).show();
			}
		catch( Exception e) {
			Toast.makeText( getApplicationContext(), "Couldn't send SMS, try again!", Toast.LENGTH_LONG ).show();
		}
		
		stopSelf();
		return Service.START_REDELIVER_INTENT;
	}

	@Override
	public IBinder onBind( Intent intent ){
		return null;
	}
}
