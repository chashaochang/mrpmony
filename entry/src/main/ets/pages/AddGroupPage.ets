import { common, wantConstant } from '@kit.AbilityKit'

@ComponentV2
export struct AddGroupPage {
  private readonly groupNumber: string = '1095261513'

  private openQQGroup(): void {
    try {
      const abilityContext = AppStorage.get('abilityContext') as common.UIAbilityContext
      abilityContext.startAbility({
        uri: `mqqapi://card/show_pslcard?src_type=internal&version=1&uin=${this.groupNumber}&card_type=group&source=qrcode`,
        flags: wantConstant.Flags.FLAG_START_WITHOUT_TIPS
      }).catch((error: Error) => {
        console.warn(`open qq group failed: ${error.message}`)
      })
    } catch (error) {
      console.warn(`open qq group exception: ${error instanceof Error ? error.message : JSON.stringify(error)}`)
    }
  }

  build() {
      Column() {
        Blank().height(70)
        Column() {
          Image($r('app.media.qrcode')).width(240).borderRadius(16).padding(8).backgroundColor('#fcfcfc')
          Blank().height(30)
          Text('扫描二维码加入群聊').fontSize(16).fontColor($r('sys.color.ohos_id_color_text_primary'))
          Blank().height(20)
        }.width('100%')
        .padding(20)
        .borderRadius(20)
        .backgroundColor($r('app.color.app_surface'))

        Blank()
      }.width('100%').height('100%').backgroundColor($r('app.color.start_window_background')).padding({
        left: 16, right: 16
      })
    }

}
