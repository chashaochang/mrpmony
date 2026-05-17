import UIAbility from '@ohos.app.ability.UIAbility';
import type { Configuration } from '@ohos.app.ability.Configuration';
import ConfigurationConstant from '@ohos.app.ability.ConfigurationConstant';
import window from '@ohos.window';
import hilog from '@ohos.hilog';
import { configureRuntimeAssets } from '../adapters/RuntimeAssetBootstrap';
import { configureAppPreferences } from '../services/AppPreferences';

const DOMAIN = 0xFF00;
const TAG = 'EntryAbility';

export default class EntryAbility extends UIAbility {
  private mainWindow?: window.Window;

  onWindowStageCreate(windowStage: window.WindowStage): void {
    AppStorage.setOrCreate('abilityContext', this.context);
    configureAppPreferences(this.context);
    configureRuntimeAssets(this.context);
    this.mainWindow = windowStage.getMainWindowSync();
    void this.mainWindow.setWindowLayoutFullScreen(true)
      .then(() => this.applySystemBarStyle(this.context.config.colorMode))
      .catch((error: Error) => {
        hilog.warn(DOMAIN, TAG, 'Failed to set immersive system bar style: %{public}s', error.message);
      });
    windowStage.loadContent('pages/ShellRootPage', (err) => {
      if (err.code) {
        hilog.error(DOMAIN, TAG, 'Failed to load ShellRootPage: %{public}s', JSON.stringify(err));
        return;
      }
      hilog.info(DOMAIN, TAG, 'ShellRootPage loaded');
    });
  }

  onConfigurationUpdate(newConfig: Configuration): void {
    this.applySystemBarStyle(newConfig.colorMode);
  }

  private applySystemBarStyle(colorMode?: ConfigurationConstant.ColorMode): void {
    if (!this.mainWindow) {
      return;
    }
    const isDark = colorMode === ConfigurationConstant.ColorMode.COLOR_MODE_DARK;
    void this.mainWindow.setWindowSystemBarProperties({
      statusBarColor: '#00000000',
      statusBarContentColor: isDark ? '#F5FBFF' : '#111820',
      navigationBarColor: '#00000000',
      navigationBarContentColor: isDark ? '#F5FBFF' : '#111820',
      enableStatusBarAnimation: false,
      enableNavigationBarAnimation: false
    }).catch((error: Error) => {
      hilog.warn(DOMAIN, TAG, 'Failed to update system bar style: %{public}s', error.message);
    });
  }
}
