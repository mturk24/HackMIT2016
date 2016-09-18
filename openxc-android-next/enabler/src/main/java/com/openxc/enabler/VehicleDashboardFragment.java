package com.openxc.enabler;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.support.v4.app.ListFragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.google.firebase.database.DataSnapshot;
import com.google.firebase.database.DatabaseError;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;
import com.google.firebase.iid.FirebaseInstanceId;
import com.google.gson.Gson;
import com.openxc.VehicleManager;
import com.openxc.messages.EventedSimpleVehicleMessage;
import com.openxc.messages.SimpleVehicleMessage;
import com.openxc.messages.VehicleMessage;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;


public class VehicleDashboardFragment extends ListFragment {
    private static String TAG = "VehicleDashboard";

    private VehicleManager mVehicleManager;
    private SimpleVehicleMessageAdapter mAdapter;
//    @Override
//    public void onComplete(DatabaseError databaseError, boolean b,
//                           DataSnapshot dataSnapshot) {
//        // Transaction completed
//        Log.d(TAG, "postTransaction:onComplete:" + databaseError);
//    }
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mAdapter = new SimpleVehicleMessageAdapter(getActivity());



        String refreshedToken = FirebaseInstanceId.getInstance().getToken();
        Log.d(TAG, "Refreshed token: " + refreshedToken);
        try {
            JSONObject mergedObj = new JSONObject();
            JSONObject obj1 = new JSONObject();
            JSONObject obj2 = new JSONObject();
            JSONObject obj3 = new JSONObject();
            JSONObject obj4 = new JSONObject();
            JSONObject obj5 = new JSONObject();
            JSONObject obj6 = new JSONObject();
            JSONObject obj7 = new JSONObject();
            obj1.put("name", "fine_odometer_since_restart");
            obj1.put("value", "0.0");
            obj2.put("name", "accelerator_pedal_position");
            obj2.put("value", "0.0");
            obj3.put("name", "powertrain_torque");
            obj3.put("value", "-14.0");
            obj4.put("name", "latitude");

            obj4.put("value", "42.293079");
            obj5.put("name", "longitude");
            obj5.put("value", "-83.237427");
            obj6.put("name", "steering_wheel_angle");
            obj6.put("value", "-348.171906");
            obj7.put("name", "engine_speed");
            obj7.put("value", "0.0");



            Iterator i1 = obj1.keys();
            Iterator i2 = obj2.keys();
            Iterator i3 = obj3.keys();
            Iterator i4 = obj4.keys();
            Iterator i5 = obj5.keys();
            Iterator i6 = obj6.keys();
            Iterator i7 = obj7.keys();

            String tmp_key;
            while(i1.hasNext()) {
                tmp_key = (String) i1.next();
                mergedObj.put(tmp_key, obj1.get(tmp_key));
            }
            while(i2.hasNext()) {
                tmp_key = (String) i2.next();
                mergedObj.put(tmp_key, obj2.get(tmp_key));
            }
            while(i3.hasNext()) {
                tmp_key = (String) i3.next();
                mergedObj.put(tmp_key, obj3.get(tmp_key));
            }
            while(i4.hasNext()) {
                tmp_key = (String) i4.next();
                mergedObj.put(tmp_key, obj4.get(tmp_key));
            }
            while(i5.hasNext()) {
                tmp_key = (String) i5.next();
                mergedObj.put(tmp_key, obj5.get(tmp_key));
            }
            while(i6.hasNext()) {
                tmp_key = (String) i6.next();
                mergedObj.put(tmp_key, obj6.get(tmp_key));
            }
            while(i7.hasNext()) {
                tmp_key = (String) i7.next();
                mergedObj.put(tmp_key, obj7.get(tmp_key));
            }
//                JSONObject obj = new JSONObject();
//                JSONObject Obj1 = (JSONObject) jso1.get("Object1");
//                JSONObject Obj2 = (JSONObject) jso2.get("Object2");
//                JSONObject combined = new JSONObject();
//                combined.put("Object1", Obj1);
//                combined.put("Object2", Obj2);
//
//                obj.put("Name", "crunchify.com");
//                obj.put("Author", "App Shah");

            Gson gson = new Gson();
            myPojo myP = gson.fromJson(mergedObj.toString(), myPojo.class);
            FirebaseDatabase database = FirebaseDatabase.getInstance();
            DatabaseReference myRef = database.getReference("car-data");

            myRef.setValue(myP);
        } catch (JSONException e) {
            Log.e("MYAPP", "unexpected JSON exception", e);
            // Do something to recover ... or kill the app.
        }
//        try {
//        FirebaseDatabase database = FirebaseDatabase.getInstance();
//        DatabaseReference myRef = database.getReference("car-data");
//
//        Map<String, JSONObject> userMap= new HashMap<String, JSONObject>();
//        JSONObject tempObject = new JSONObject();
//        tempObject.put("birthday","1992");
//        tempObject.put("fullName","Emin AYAR");
//        userMap.put("myUser", tempObject); myRef.setValue(userMap);
//        } catch (JSONException e) {
////            Log.e("MYAPP", "unexpected JSON exception", e);
////            // Do something to recover ... or kill the app.
//        }








    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        View v = inflater.inflate(R.layout.vehicle_dashboard, container, false);
        return v;
    }

    @Override
    public void setUserVisibleHint(boolean isVisibleToUser) {
        super.setUserVisibleHint(isVisibleToUser);
        if(getActivity() == null) {
            return;
        }

        if (isVisibleToUser) {
            getActivity().bindService(
                    new Intent(getActivity(), VehicleManager.class),
                    mConnection, Context.BIND_AUTO_CREATE);
        } else {
            if(mVehicleManager != null) {
                Log.i(TAG, "Unbinding from vehicle service");
                mVehicleManager.removeListener(SimpleVehicleMessage.class, mListener);
                mVehicleManager.removeListener(EventedSimpleVehicleMessage.class, mListener);
                getActivity().unbindService(mConnection);
                mVehicleManager = null;
            }
        }
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        setListAdapter(mAdapter);
    }

    private VehicleMessage.Listener mListener = new VehicleMessage.Listener() {
        @Override
        public void receive(final VehicleMessage message) {
            Activity activity = getActivity();
            if(activity != null) {
                getActivity().runOnUiThread(new Runnable() {
                    public void run() {
                        if(message instanceof EventedSimpleVehicleMessage) {
                            SimpleVehicleMessage convertedMsg = new SimpleVehicleMessage(message.getTimestamp(),
                                    ((EventedSimpleVehicleMessage) message).getName(),
                                    ((EventedSimpleVehicleMessage) message).getValue() +
                                            ": " + ((EventedSimpleVehicleMessage) message).getEvent());
                            mAdapter.add(convertedMsg.asSimpleMessage());
                        }
                        else
                            mAdapter.add(message.asSimpleMessage());
                    }
                });
            }
        }
    };

    private ServiceConnection mConnection = new ServiceConnection() {
        public void onServiceConnected(ComponentName className,
                IBinder service) {
            Log.i(TAG, "Bound to VehicleManager");
            mVehicleManager = ((VehicleManager.VehicleBinder)service
                    ).getService();

            mVehicleManager.addListener(SimpleVehicleMessage.class, mListener);
            mVehicleManager.addListener(EventedSimpleVehicleMessage.class, mListener);
        }

        public void onServiceDisconnected(ComponentName className) {
            Log.w(TAG, "VehicleService disconnected unexpectedly");
            mVehicleManager = null;
        }
    };
}
