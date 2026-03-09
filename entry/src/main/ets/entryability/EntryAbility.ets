import UIAbility from '@ohos.app.ability.UIAbility';
import window from '@ohos.window';
import hilog from '@ohos.hilog';

const DOMAIN = 0xFF00;
const TAG = 'EntryAbility';

export default class EntryAbility extends UIAbility {
  onWindowStageCreate(windowStage: window.WindowStage): void {
    windowStage.loadContent('pages/ShellRootPage', (err) => {
      if (err.code) {
        hilog.error(DOMAIN, TAG, 'Failed to load ShellRootPage: %{public}s', JSON.stringify(err));
        return;
      }
      hilog.info(DOMAIN, TAG, 'ShellRootPage loaded');
    });
  }
}
