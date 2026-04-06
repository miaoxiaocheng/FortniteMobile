package load.tencent.lib;

import android.content.Context;
import android.graphics.Typeface;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.GradientDrawable;
import android.os.Build;
import android.util.DisplayMetrics;
import android.view.Gravity;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

public class FloatContentView extends PopupWindow {
    static {
        System.loadLibrary("Syxy");//加载库
    }

    private Context mContext;
    private EditText inputFieldFps; // 帧率输入框
    private EditText inputField3D;  // 3D分辨率输入框

    public FloatContentView(Context context) {
        super(context);
        this.mContext = context;
        initView();
    }

    // JNI 方法声明
    public native boolean aw1(float value); // 修改帧率，返回是否成功
    public native void aw2(float value);    // 修改3D分辨率

    private void initView() {
        LinearLayout main = new LinearLayout(mContext);
        LinearLayout.LayoutParams mainParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.MATCH_PARENT);
        main.setLayoutParams(mainParams);

        GradientDrawable mainBackground = new GradientDrawable();
        mainBackground.setColor(0xCCFFFFFF);
        mainBackground.setCornerRadius(30);
        mainBackground.setStroke(5, 0x00000000);
        main.setBackgroundDrawable(mainBackground);

        LinearLayout mainLayout = new LinearLayout(mContext);
        LinearLayout.LayoutParams mainLayoutParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.MATCH_PARENT);
        mainLayout.setLayoutParams(mainLayoutParams);
        mainLayout.setOrientation(LinearLayout.VERTICAL);
        main.addView(mainLayout);

        LinearLayout titleLayout = new LinearLayout(mContext);
        LinearLayout.LayoutParams titleLayoutParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        titleLayout.setLayoutParams(titleLayoutParams);
        titleLayout.setGravity(Gravity.CENTER);
        titleLayout.setPadding(20, 20, 20, 20);
        mainLayout.addView(titleLayout);
   
//------------------------------------------------------------------------------------------------------------------------------------------
        TextView title = new TextView(mContext);
        LinearLayout.LayoutParams titleParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        title.setLayoutParams(titleParams);
        title.setText("FortniteTools by.妙小橙");
        title.setTextSize(18);
        title.setTextColor(0x800085FF);
        title.setTypeface(Typeface.defaultFromStyle(Typeface.BOLD));
        titleLayout.addView(title);

        // 在标题下方添加QQ群号显示
        TextView groupInfo = new TextView(mContext);
        LinearLayout.LayoutParams groupParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        groupParams.setMargins(0, 5, 0, 15); // 设置与标题和分隔线的间距
        groupInfo.setLayoutParams(groupParams);
        groupInfo.setText("QQ群：916981590"); // 设置显示的群号
        groupInfo.setTextSize(16); // 设置文字大小
        groupInfo.setTextColor(0xFF666666); // 设置文字颜色为深灰色
        groupInfo.setGravity(Gravity.CENTER); // 居中显示
        mainLayout.addView(groupInfo); // 添加到标题所在的主纵向布局中

        LinearLayout line_view = new LinearLayout(mContext);
        LinearLayout.LayoutParams line_view_params = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, 2);
        line_view_params.leftMargin = 20;
        line_view_params.rightMargin = 20;
        line_view.setLayoutParams(line_view_params);
        line_view.setBackgroundColor(0xA0000000);
        mainLayout.addView(line_view);

        ScrollView scroll = new ScrollView(mContext);
        ScrollView.LayoutParams scrollParams = new ScrollView.LayoutParams(ScrollView.LayoutParams.MATCH_PARENT, ScrollView.LayoutParams.WRAP_CONTENT);
        scrollParams.bottomMargin = 10;
        scroll.setLayoutParams(scrollParams);
        scroll.setVerticalScrollBarEnabled(false);
        mainLayout.addView(scroll);

        LinearLayout layout = new LinearLayout(mContext);
        LinearLayout.LayoutParams layoutParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        layout.setLayoutParams(layoutParams);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(20, 0, 20, 20);
        scroll.addView(layout);

        // 添加功能标题
        TextView functionTitle = new TextView(mContext);
        LinearLayout.LayoutParams functionTitleParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        functionTitleParams.setMargins(0, 0, 0, 10);
        functionTitle.setLayoutParams(functionTitleParams);
        functionTitle.setText("帧率修改");
        functionTitle.setTextSize(16);
        functionTitle.setTextColor(0xFF0085FF);
        functionTitle.setTypeface(Typeface.defaultFromStyle(Typeface.BOLD));
        layout.addView(functionTitle);

        // 帧率修改输入框
        inputFieldFps = new EditText(mContext);
        inputFieldFps.setHint("输入数字：60 90 120 144 165");
        inputFieldFps.setInputType(android.text.InputType.TYPE_CLASS_NUMBER | android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL);
        inputFieldFps.setSingleLine(true);
        GradientDrawable inputBackgroundFps = new GradientDrawable();
        inputBackgroundFps.setColor(0xFFFFFFFF);
        inputBackgroundFps.setCornerRadius(20);
        inputBackgroundFps.setStroke(3, 0xFF0085FF);
        inputFieldFps.setBackground(inputBackgroundFps);
        LinearLayout.LayoutParams inputParamsFps = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        inputParamsFps.setMargins(0, 0, 0, 20);
        inputFieldFps.setLayoutParams(inputParamsFps);
        layout.addView(inputFieldFps);

        // 帧率修改按钮
        Button modifyButtonFps = new Button(mContext);
        LinearLayout.LayoutParams buttonParamsFps = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        buttonParamsFps.gravity = Gravity.CENTER;
        modifyButtonFps.setLayoutParams(buttonParamsFps);
        modifyButtonFps.setText("修改帧率");
        modifyButtonFps.setTextColor(0xFFFFFFFF);
        modifyButtonFps.setTypeface(Typeface.defaultFromStyle(Typeface.BOLD));
        GradientDrawable buttonBackgroundFps = new GradientDrawable();
        buttonBackgroundFps.setColor(0xFF0085FF);
        buttonBackgroundFps.setCornerRadius(20);
        modifyButtonFps.setBackground(buttonBackgroundFps);
        layout.addView(modifyButtonFps);

        // 添加分隔线
        LinearLayout separator = new LinearLayout(mContext);
        LinearLayout.LayoutParams separatorParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, 2);
        separatorParams.setMargins(0, 20, 0, 20);
        separator.setLayoutParams(separatorParams);
        separator.setBackgroundColor(0x30000000);
        layout.addView(separator);

        // 3D分辨率功能标题
        TextView functionTitle3D = new TextView(mContext);
        LinearLayout.LayoutParams functionTitle3DParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        functionTitle3DParams.setMargins(0, 0, 0, 10);
        functionTitle3D.setLayoutParams(functionTitle3DParams);
        functionTitle3D.setText("3D分辨率修改");
        functionTitle3D.setTextSize(16);
        functionTitle3D.setTextColor(0xFFFF6B6B);
        functionTitle3D.setTypeface(Typeface.defaultFromStyle(Typeface.BOLD));
        layout.addView(functionTitle3D);

        // 3D分辨率修改输入框
        inputField3D = new EditText(mContext);
        inputField3D.setHint("输入数字：100 120 150 180 200");
        inputField3D.setInputType(android.text.InputType.TYPE_CLASS_NUMBER | android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL | android.text.InputType.TYPE_NUMBER_FLAG_SIGNED);
        inputField3D.setSingleLine(true);
        GradientDrawable inputBackground3D = new GradientDrawable();
        inputBackground3D.setColor(0xFFFFFFFF);
        inputBackground3D.setCornerRadius(20);
        inputBackground3D.setStroke(3, 0xFFFF6B6B);
        inputField3D.setBackground(inputBackground3D);
        LinearLayout.LayoutParams inputParams3D = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        inputParams3D.setMargins(0, 0, 0, 20);
        inputField3D.setLayoutParams(inputParams3D);
        layout.addView(inputField3D);

        // 3D分辨率修改按钮
        Button modifyButton3D = new Button(mContext);
        LinearLayout.LayoutParams buttonParams3D = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        buttonParams3D.gravity = Gravity.CENTER;
        modifyButton3D.setLayoutParams(buttonParams3D);
        modifyButton3D.setText("修改3D分辨率");
        modifyButton3D.setTextColor(0xFFFFFFFF);
        modifyButton3D.setTypeface(Typeface.defaultFromStyle(Typeface.BOLD));
        GradientDrawable buttonBackground3D = new GradientDrawable();
        buttonBackground3D.setColor(0xFFFF6B6B);
        buttonBackground3D.setCornerRadius(20);
        modifyButton3D.setBackground(buttonBackground3D);
        layout.addView(modifyButton3D);

        WindowManager windowManager = (WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE);
        DisplayMetrics metrics = new DisplayMetrics();
        windowManager.getDefaultDisplay().getRealMetrics(metrics);
        int metricsWidth = metrics.widthPixels;
        int metricsHeight = metrics.heightPixels;

        setWidth(metricsWidth > metricsHeight ? metricsWidth / 3 : metricsHeight / 3);
        setHeight(metricsWidth < metricsHeight ? metricsWidth - 50 : metricsHeight - 50);
        setContentView(main);
        setBackgroundDrawable(new ColorDrawable(0));
        setOutsideTouchable(true);
        setFocusable(true);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            setWindowLayoutType(WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY);
        } else {
            setWindowLayoutType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
        }

        // 帧率修改按钮点击事件
        modifyButtonFps.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                String inputText = inputFieldFps.getText().toString();
                if (!inputText.isEmpty()) {
                    try {
                        float value = Float.parseFloat(inputText);
                        boolean success = aw1(value); // 调用 JNI 方法
                        if (success) {
                            showToast("帧率修改成功，值为: " + value);
                        } else {
                            showToast("帧率修改失败");
                        }
                    } catch (NumberFormatException e) {
                        showToast("请输入有效的数字");
                    }
                } else {
                    showToast("请输入帧率值");
                }
            }
        });

        // 3D分辨率修改按钮点击事件
        modifyButton3D.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                String inputText = inputField3D.getText().toString();
                if (!inputText.isEmpty()) {
                    try {
                        float value = Float.parseFloat(inputText);
                        aw2(value); // 调用 JNI 方法
                        showToast("3D分辨率修成功，值为: " + value);
                    } catch (NumberFormatException e) {
                        showToast("请输入有效的数字");
                    }
                } else {
                    showToast("请输入3D分辨率值");
                }
            }
        });
    }

    private void showToast(String str) {
        Toast.makeText(mContext, str, Toast.LENGTH_LONG).show();
    }

    public void showView() {
        this.showAtLocation(this.getContentView(), Gravity.CENTER, 0, 0);
    }
}
