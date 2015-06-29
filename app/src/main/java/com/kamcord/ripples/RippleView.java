package com.kamcord.ripples;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.opengl.GLSurfaceView;
import android.view.MotionEvent;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import com.kamcord.ripples.R;

class RippleView extends GLSurfaceView
{
	public enum GLVersion { ES_1_1, ES_2_0, ES_3_0 };

	private Context context;
	private GLVersion version;

    public RippleView(Context context, GLVersion version)
    {	
        super(context);
        this.context = context;
        this.version = version;
        init();
    }

    public RippleView(Context context, boolean translucent, int depth, int stencil, GLVersion version)
    {
        super(context);
        this.context = context;
        this.version = version;
        init();
    }
    
    
    private void init()
    {
    	int contextClientVersion = 1;
    	if ( GLVersion.ES_2_0 == version ) contextClientVersion = 2;
        else if ( GLVersion.ES_3_0 == version ) contextClientVersion = 3;

    	setEGLContextClientVersion(contextClientVersion);
        setRenderer(new Renderer(context, version));
        setRenderMode(RENDERMODE_CONTINUOUSLY);
    }

    private static class Renderer implements GLSurfaceView.Renderer
    {
        public Context context;
        public GLVersion version;

        public Renderer(Context context, GLVersion version)
        {
            this.context = context;
            this.version = version;
        }

        public void onDrawFrame(GL10 egl)
        {
            RippleLib.DrawRipple(version.ordinal());
        }
       
       
        private boolean newOpenGLContext;

        public void onSurfaceCreated(GL10 gl, EGLConfig config)
        {
            newOpenGLContext = true;
        }

        private int lastWidth;
        private int lastHeight;

        public void onSurfaceChanged(GL10 gl, int width, int height)
        {
            boolean sizeChanged = (lastWidth != width || lastHeight != height);

            if( !newOpenGLContext && sizeChanged )
                RippleLib.DestroyRipple();

            if( newOpenGLContext || sizeChanged )
            {
                BitmapFactory.Options opts = new BitmapFactory.Options();
                opts.inScaled = false;

                Bitmap temp = BitmapFactory.decodeResource(
                        context.getResources(), R.drawable.kamcord_underwater, opts);
                int bw = temp.getWidth();
                int bh = temp.getHeight();
                int pixels[] = new int[bh * bw];
                temp.getPixels(pixels, 0, bw, 0, 0, bw, bh);

                RippleLib.InitRipple(width, height, bw, bh, pixels, version.ordinal());

                lastWidth = width;
                lastHeight = height;
            }

            newOpenGLContext = false;
        }
    }

    public boolean onTouchEvent(MotionEvent e)
    {
        switch( e.getAction() )
        {
        case MotionEvent.ACTION_DOWN:
        case MotionEvent.ACTION_MOVE:
            RippleLib.TouchRipple(e.getX(), e.getY());
            break;
        }

        return true;
    }
}
