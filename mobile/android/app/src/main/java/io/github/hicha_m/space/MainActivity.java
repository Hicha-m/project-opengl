package io.github.hicha_m.space;
import org.libsdl.app.SDLActivity;
public class MainActivity extends SDLActivity {
    @Override protected String[] getArguments() {
        if (getIntent().getBooleanExtra("mobile_test", false)) return new String[] {"--mobile-test", "--keep-running"};
        return getIntent().getBooleanExtra("smoke_test", false) ? new String[] {"--smoke-test"} : new String[0];
    }
    @Override protected String[] getLibraries() { return new String[] {"SDL3", "project"}; }
}
