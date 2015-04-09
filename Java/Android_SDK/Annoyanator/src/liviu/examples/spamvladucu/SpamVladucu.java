package liviu.examples.spamvladucu;

import android.telephony.SmsManager;
import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.widget.EditText;
import android.widget.Button;
import android.view.View.OnClickListener;
import android.content.Intent;
import android.widget.Toast;
import android.os.Handler;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Context;

public class SpamVladucu extends Activity{
	EditText editMessage;
	EditText editPhoneNumber;
	EditText editHowMany;
	EditText editHowOften;
	Button startTheSpam;
	String phoneNumber = null;
	String smsMessage = null;
	String howMany = null;
	String howOften = null;

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState){
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);

		editMessage = (EditText) findViewById(R.id.message);
		editPhoneNumber = (EditText) findViewById(R.id.phone_num);
		editHowMany = (EditText) findViewById(R.id.how_many);
		editHowOften = (EditText) findViewById(R.id.how_often);
		startTheSpam = (Button) findViewById(R.id.start_spam);
	
		startTheSpam.setOnClickListener(new OnClickListener(){
			@Override
			public void onClick(View v){
				phoneNumber = editPhoneNumber.getText().toString();
				smsMessage = editMessage.getText().toString();
				howMany = editHowMany.getText().toString();
				howOften = editHowOften.getText().toString();

				Intent intent = new Intent(SpamVladucu.this, SpamVladucuService.class);
				intent.putExtra("phoneNumber",phoneNumber);
				intent.putExtra("smsMessage",smsMessage);
			
				PendingIntent pintent = PendingIntent.getService(SpamVladucu.this, 0, intent, 0);
				AlarmManager alarm = (AlarmManager) getSystemService(Context.ALARM_SERVICE);
				alarm.cancel(pintent);
				alarm.setRepeating(AlarmManager.RTC_WAKEUP, System.currentTimeMillis(), 1000*60*Integer.parseInt(howOften), pintent);
			}
		});
	}
}
