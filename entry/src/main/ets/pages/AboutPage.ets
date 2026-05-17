import { bundleManager } from '@kit.AbilityKit'

@ComponentV2
export struct AboutPage {
  @Param scroller: Scroller = new Scroller()
  @Param onOpenPrivacyPolicy?: () => void = undefined
  @Param onOpenGroupPage?: () => void = undefined
  @Local appVersion: string = '--'

  aboutToAppear(): void {
    this.loadAppVersion()
  }

  private loadAppVersion(): void {
    try {
      const info = bundleManager.getBundleInfoForSelfSync(bundleManager.BundleFlag.GET_BUNDLE_INFO_DEFAULT)
      this.appVersion = info.versionName && info.versionName.length > 0 ? info.versionName : '--'
    } catch (error) {
      console.warn(`load app version failed: ${error instanceof Error ? error.message : JSON.stringify(error)}`)
      this.appVersion = '--'
    }
  }

  @Builder
  private TopSafeGapItem() {
    ListItem() {
      Column()
        .width('100%')
        .height(74)
    }
    .selectable(false)
  }

  @Builder
  private SectionTitle(title: string) {
    ListItem() {
      Text(title)
        .fontSize(18)
        .fontWeight(FontWeight.Medium)
        .fontColor($r('sys.color.ohos_id_color_text_primary'))
        .width('100%')
        .padding({ top: 14, bottom: 8 })
    }
    .selectable(false)
  }

  @Builder
  private ValueBadge(value: string) {
    Text(value)
      .fontSize(11)
      .fontWeight(FontWeight.Medium)
      .fontColor($r('app.color.app_accent_text'))
      .maxLines(1)
      .textOverflow({ overflow: TextOverflow.Ellipsis })
      .height(26)
      .padding({ left: 9, right: 9 })
      .borderRadius(13)
      .backgroundColor($r('app.color.app_surface_muted'))
  }

  @Builder
  private AboutRow(title: string, value?: string, detail?: string, showDivider: boolean = true, onClick?: () => void) {
    Column() {
      Row({ space: 10 }) {
        Column({ space: detail && detail.length > 0 ? 3 : 0 }) {
          Text(title)
            .fontSize(14)
            .fontWeight(FontWeight.Medium)
            .fontColor($r('sys.color.ohos_id_color_text_primary'))
            .maxLines(1)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
            .width('100%')

          if (detail && detail.length > 0) {
            Text(detail)
              .fontSize(11)
              .fontColor($r('sys.color.ohos_id_color_text_secondary'))
              .maxLines(2)
              .textOverflow({ overflow: TextOverflow.Ellipsis })
              .width('100%')
          }
        }
        .layoutWeight(1)
        .alignItems(HorizontalAlign.Start)
        .constraintSize({ minWidth: 0 })

        if (value && value.length > 0) {
          this.ValueBadge(value)
        }
      }
      .width('100%')
      .padding({ left: 12, right: 12, top: 14, bottom: 14 })
      .alignItems(VerticalAlign.Center)
      .onClick(() => {
        if (onClick) {
          onClick()
        }
      })

      if (showDivider) {
        Row() {
          Text('')
            .height(1)
            .layoutWeight(1)
            .backgroundColor($r('sys.color.ohos_id_color_list_separator'))
        }
        .width('100%')
        .padding({ left: 12, right: 12 })
      }
    }
    .width('100%')
  }

  @Builder
  private AppInfoCard() {
    ListItem() {
      Column({ space: 0 }) {
        this.AboutRow('应用名称', 'mrpmony')
        this.AboutRow('当前版本', this.appVersion, undefined, false)
      }
      .width('100%')
      .backgroundColor($r('app.color.app_surface'))
      .border({ width: 1, color: $r('app.color.app_surface_border') })
      .borderRadius(22)
      .shadow({
        radius: 18,
        color: '#24385022',
        offsetX: 0,
        offsetY: 8
      })
      .margin({ top: 2, bottom: 8 })
    }
    .selectable(false)
  }

  @Builder
  private OpenSourceCard() {
    ListItem() {
      Column({ space: 0 }) {
        this.AboutRow('开源组件', 'vmrp / Unicorn')
        this.AboutRow('源码与许可', '项目仓库提供', '对应源码、许可证文本和第三方说明随开源仓库或发行页提供。', false)
      }
      .width('100%')
      .backgroundColor($r('app.color.app_surface'))
      .border({ width: 1, color: $r('app.color.app_surface_border') })
      .borderRadius(22)
      .shadow({
        radius: 18,
        color: '#24385022',
        offsetX: 0,
        offsetY: 8
      })
      .margin({ top: 2, bottom: 8 })
    }
    .selectable(false)
  }

  @Builder
  private PrivacyCard() {
    ListItem() {
      Column({ space: 0 }) {
        this.AboutRow('隐私政策', '查看', 'AppGallery 托管页面', false, () => {
          if (this.onOpenPrivacyPolicy) {
            this.onOpenPrivacyPolicy()
          }
        })
      }
      .width('100%')
      .backgroundColor($r('app.color.app_surface'))
      .border({ width: 1, color: $r('app.color.app_surface_border') })
      .borderRadius(22)
      .shadow({
        radius: 18,
        color: '#24385022',
        offsetX: 0,
        offsetY: 8
      })
      .margin({ top: 2, bottom: 8 })
    }
    .selectable(false)
  }

  @Builder
  private GroupCard() {
    ListItem() {
      Column({ space: 0 }) {
        this.AboutRow('QQ群聊', '加入', '', false, () => {
          if (this.onOpenGroupPage) {
            this.onOpenGroupPage()
          }
        })
      }
      .width('100%')
      .backgroundColor($r('app.color.app_surface'))
      .border({ width: 1, color: $r('app.color.app_surface_border') })
      .borderRadius(22)
      .shadow({
        radius: 18,
        color: '#24385022',
        offsetX: 0,
        offsetY: 8
      })
      .margin({ top: 2, bottom: 8 })
    }
    .selectable(false)
  }

  @Builder
  private DisclaimerCard() {
    ListItem() {
      Column({ space: 0 }) {
        this.AboutRow('兼容性', '', '不承诺兼容所有 MRP 应用、游戏、插件或存档。')
        this.AboutRow('内容', '', '本应用不提供任何资源,需用户自行导入.',false)
        //this.AboutRow('官方关系', '无官方关联', '与 Mythroad、相关厂商或版权方无官方隶属、授权或背书关系。', false)
      }
      .width('100%')
      .backgroundColor($r('app.color.app_surface'))
      .border({ width: 1, color: $r('app.color.app_surface_border') })
      .borderRadius(22)
      .shadow({
        radius: 18,
        color: '#24385022',
        offsetX: 0,
        offsetY: 8
      })
      .margin({ top: 2, bottom: 8 })
    }
    .selectable(false)
  }

  @Builder
  private BottomSafeTail() {
    ListItem() {
      Column()
        .width('100%')
        .height(116)
    }
    .selectable(false)
  }

  build() {
    List({ scroller: this.scroller }) {
      this.TopSafeGapItem()

      this.SectionTitle('应用信息')
      this.AppInfoCard()

      // this.SectionTitle('开源信息')
      // this.OpenSourceCard()

      this.SectionTitle('隐私与内容')
      this.PrivacyCard()

      this.SectionTitle('免责声明')
      this.DisclaimerCard()

      this.SectionTitle('反馈交流')
      this.GroupCard()

      this.BottomSafeTail()
    }
    .width('100%')
    .height('100%')
    .padding({ left: 16, right: 16 })
    .edgeEffect(EdgeEffect.Spring)
    .linearGradient({
      angle: 180,
      colors: [
        [$r('app.color.home_background_start'), 0.0],
        [$r('app.color.home_background_mid'), 0.48],
        [$r('app.color.home_background_end'), 1.0]
      ]
    })
    .expandSafeArea([SafeAreaType.SYSTEM], [SafeAreaEdge.TOP, SafeAreaEdge.BOTTOM])
  }
}
