import common from '@ohos.app.ability.common'
import window from '@ohos.window'
import {
  DividerShowType,
  HdsNavDestination,
  HdsNavDestinationTitleMode,
  HdsNavigationMenuItemOptions,
  HdsNavigation,
  HdsNavigationTitleMode,
  HdsTabs,
  HdsTabsController,
  ScrollEffectType,
  TitleSize
} from '@hms.hds.hdsBaseComponent'
import { hdsMaterial } from '@hms.hds.hdsMaterial'
import { AppListPage } from './AppListPage'
import { AboutPage } from './AboutPage'
import { AddGroupPage } from './AddGroupPage'
import { AppManagePage } from './AppManagePage'
import { ImportPage } from './ImportPage'
import { RecentRunPage } from './RecentRunPage'
import { RunPage } from './RunPage'
import { SettingsPage } from './SettingsPage'
import { WebPage } from './WebPage'
import { AppPreferences } from '../services/AppPreferences'
import { RecentRunRecorder } from '../services/RecentRunRecorder'
import { RuntimeService } from '../services/RuntimeService'

interface RunPageParam {
  appId: string
}

interface RunSettingsPageParam {
  appId: string
}

interface WebPageParam {
  title: string
  url: string
}

@Entry
@ComponentV2
struct ShellRootPage {
  @Local catalogRefreshTick: number = 0
  @Local recentRefreshTick: number = 0
  @Local selectedTabIndex: number = 0
  @Local recentAppIdsText: string = ''
  @Provider() navigationTopSafeGap: number = 0
  @Local runtimeSettingsRevision: number = 0
  @Local runtimeDestinationVisible: boolean = true
  @Local runtimeDestinationVisibilityRevision: number = 0
  @Local materialLevel: hdsMaterial.MaterialLevel = hdsMaterial.MaterialLevel.ADAPTIVE
  @Local materialType: hdsMaterial.MaterialType = hdsMaterial.MaterialType.IMMERSIVE
  private pathStack: NavPathStack = new NavPathStack()
  private tabsController: HdsTabsController = new HdsTabsController()
  private appListScroller: Scroller = new Scroller()
  private recentScroller: Scroller = new Scroller()
  private settingsScroller: Scroller = new Scroller()
  private importScroller: Scroller = new Scroller()
  private manageScroller: Scroller = new Scroller()
  private runSettingsScroller: Scroller = new Scroller()
  private appPreferences: AppPreferences = new AppPreferences()
  private recentRunRecorder: RecentRunRecorder = new RecentRunRecorder()
  private runtimeControlService: RuntimeService = new RuntimeService()

  aboutToAppear(): void {
    this.recentAppIdsText = this.appPreferences.recentAppIdsText()
    this.recentRunRecorder.configure(this.recentAppIdsText, (recentAppIdsText: string) => {
      this.recentAppIdsText = recentAppIdsText
      this.recentRefreshTick += 1
      this.appPreferences.saveRecentAppIdsText(recentAppIdsText)
    })
    this.syncNavigationTopSafeGap()
  }

  private goToImport(): void {
    this.pathStack.pushPathByName('import', new Object())
  }

  private goToManage(): void {
    this.pathStack.pushPathByName('manage', new Object())
  }

  private goToRun(appId: string): void {
    this.runtimeDestinationVisible = true
    this.runtimeDestinationVisibilityRevision += 1
    const params: RunPageParam = { appId }
    this.pathStack.pushPathByName('run', params)
  }

  private goToRunSettings(appId: string): void {
    const params: RunSettingsPageParam = { appId }
    this.pathStack.pushPathByName('runSettings', params)
  }

  private goToAbout(): void {
    this.pathStack.pushPathByName('about', new Object())
  }

  private goToAddGroup(): void {
    this.pathStack.pushPathByName('addGroup', new Object())
  }

  private goToPrivacyPolicy(): void {
    const params: WebPageParam = {
      title: '隐私政策',
      url: 'https://agreement-drcn.hispace.dbankcloud.cn/index.html?lang=zh&agreementId=1948187180480336512'
    }
    this.pathStack.pushPathByName('web', params)
  }

  private backFromRunSettings(): boolean {
    this.pathStack.pop()
    return true
  }

  private backToList(): void {
    this.pathStack.pop()
  }

  private backToListAfterImport(): void {
    this.catalogRefreshTick += 1
    this.pathStack.pop()
  }

  private backFromRun(): boolean {
    this.pathStack.pop()
    void this.runtimeControlService.stop()
    return true
  }

  private handleRunDestinationShown(): void {
    this.runtimeDestinationVisible = true
    this.runtimeDestinationVisibilityRevision += 1
  }

  private handleRunDestinationHidden(): void {
    this.runtimeDestinationVisible = false
    this.runtimeDestinationVisibilityRevision += 1
  }

  private runAppId(params: Object): string {
    const runParams = params as RunPageParam
    if (runParams && runParams.appId) {
      return runParams.appId
    }
    return 'dsm_gm.mrp'
  }

  private runSettingsAppId(params: Object): string {
    const runSettingsParams = params as RunSettingsPageParam
    if (runSettingsParams && runSettingsParams.appId) {
      return runSettingsParams.appId
    }
    return 'dsm_gm.mrp'
  }

  private webPageTitle(params: Object): string {
    const webParams = params as WebPageParam
    if (webParams && webParams.title) {
      return webParams.title
    }
    return '网页'
  }

  private webPageUrl(params: Object): string {
    const webParams = params as WebPageParam
    if (webParams && webParams.url) {
      return webParams.url
    }
    return ''
  }

  private rootTitle(): string {
    if (this.selectedTabIndex === 1) {
      return '最近'
    }
    if (this.selectedTabIndex === 2) {
      return '设置'
    }
    return 'mrpmony'
  }

  private homeListTopSafeGap(): number {
    return 68
  }

  private applyNavigationTopSafeGap(area: window.AvoidArea): void {
    this.navigationTopSafeGap = this.getUIContext().px2vp(area.topRect.height)
  }

  private async syncNavigationTopSafeGap(): Promise<void> {
    try {
      const context = this.getUIContext().getHostContext() as common.UIAbilityContext
      const currentWindow = await window.getLastWindow(context)
      this.applyNavigationTopSafeGap(currentWindow.getWindowAvoidArea(window.AvoidAreaType.TYPE_SYSTEM))
    } catch (error) {
      console.warn(`sync navigation safe area failed: ${error instanceof Error ? error.message : JSON.stringify(error)}`)
    }
  }

  private rootMenuItems(): Array<HdsNavigationMenuItemOptions> {
    return [
      {
        content: {
          icon: $r('sys.media.ohos_ic_public_more'),
          label: '管理',
          action: () => this.goToManage()
        }
      },
      {
        content: {
          icon: $r('sys.symbol.plus'),
          label: '导入',
          action: () => this.goToImport()
        }
      }
    ]
  }

  private runMenuItems(appId: string): Array<HdsNavigationMenuItemOptions> {
    return [{
      content: {
        icon: $r('sys.media.ohos_ic_public_more'),
        label: '设置',
        action: () => this.goToRunSettings(appId)
      }
    }]
  }

  private activeRootScroller(): Scroller {
    if (this.selectedTabIndex === 1) {
      return this.recentScroller
    }
    if (this.selectedTabIndex === 2) {
      return this.settingsScroller
    }
    return this.appListScroller
  }

  private recordRecentRun(appId: string): void {
    this.applyRecentRunPreview(appId)
    this.recentRunRecorder.record(appId)
  }

  private applyRecentRunPreview(appId: string): void {
    if (appId.length === 0) {
      return
    }
    const ids = this.recentAppIdsText.length === 0 ? [] : this.recentAppIdsText.split('|').filter((id: string) => id.length > 0 && id !== appId)
    ids.unshift(appId)
    const nextText = ids.slice(0, 20).join('|')
    if (nextText === this.recentAppIdsText) {
      this.recentRefreshTick += 1
      this.recentRunRecorder.updateCurrentText(nextText)
      return
    }
    this.recentAppIdsText = nextText
    this.recentRefreshTick += 1
    this.recentRunRecorder.updateCurrentText(nextText)
    this.appPreferences.saveRecentAppIdsText(nextText)
  }

  private clearRecentRuns(): void {
    this.recentRunRecorder.clear()
  }

  private handleAppDeleted(appId: string): void {
    this.recentRunRecorder.remove(appId)
    this.catalogRefreshTick += 1
    this.recentRefreshTick += 1
  }

  private handleAppsDeleted(appIdsText: string): void {
    this.recentRunRecorder.removeMany(appIdsText)
    this.catalogRefreshTick += 1
    this.recentRefreshTick += 1
  }

  private popDestination(): boolean {
    this.pathStack.pop()
    return true
  }

  @Builder
  private HomeTabsBuilder() {
    HdsTabs({ controller: this.tabsController }) {
      TabContent() {
        AppListPage({
          refreshToken: this.catalogRefreshTick,
          recentAppIdsText: this.recentAppIdsText,
          showImportEntryButton: false,
          scroller: this.appListScroller,
          topSafeGap: this.homeListTopSafeGap(),
          onNavigateImport: () => this.goToImport(),
          onNavigateRun: (appId: string) => this.goToRun(appId),
          onAppDeleted: (appId: string) => this.handleAppDeleted(appId)
        })
      }
      .tabBar(new BottomTabBarStyle($r('sys.media.ohos_ic_public_device_phone'), '应用'))

      TabContent() {
        RecentRunPage({
          refreshToken: this.catalogRefreshTick + this.recentRefreshTick,
          recentAppIdsText: this.recentAppIdsText,
          scroller: this.recentScroller,
          onNavigateImport: () => this.goToImport(),
          onNavigateRun: (appId: string) => this.goToRun(appId),
          onClearRecent: () => this.clearRecentRuns()
        })
      }
      .tabBar(new BottomTabBarStyle($r('sys.media.ohos_ic_public_clock'), '最近'))

      TabContent() {
        SettingsPage({
          scroller: this.settingsScroller,
          onNavigateAbout: () => this.goToAbout()
        })
      }
      .tabBar(new BottomTabBarStyle($r('sys.media.ohos_ic_public_more'), '设置'))
    }
    .width('100%')
    .height('100%')
    .barOverlap(true)
    .barPosition(BarPosition.End)
    .vertical(false)
    .barFloatingStyle({
      barWidth: {
        smallWidth: 228,
        mediumWidth: 292,
        largeWidth: 340
      },
      barBottomMargin: 24,
      systemMaterialEffect: {
        materialType: hdsMaterial.MaterialType.IMMERSIVE,
        materialLevel: hdsMaterial.MaterialLevel.ADAPTIVE
      }
    })
    .onChange((index: number) => {
      this.selectedTabIndex = index
    })
  }

  @Builder
  private DestinationBuilder(name: string, params: Object) {
    if (name === 'import') {
      HdsNavDestination() {
        ImportPage({
          scroller: this.importScroller,
          onImportSuccess: () => this.backToListAfterImport()
        })
      }
      .titleBar({
        content: {
          title: {
            mainTitle: '导入 MRP',
            mainTitleSize: TitleSize.TITLE_S
          },
          divider: {
            showType: DividerShowType.AUTO
          }
        },
        style: {
          scrollEffectOpts: {
            scrollEffectType: ScrollEffectType.IMMERSIVE_GRADIENT_BLUR
          },
          originalStyle: {
            backgroundStyle: {
              backgroundColor: $r('sys.color.ohos_id_color_titlebar_bg_transparent'),
              blurRadius: 48,
              maskExtraHeight: 32
            },
            contentStyle: {
              titleStyle: {
                mainTitleColor: $r('sys.color.ohos_id_color_titlebar_text'),
                subTitleColor: $r('sys.color.ohos_id_color_titlebar_subtitle_text')
              },
              backIconStyle: {
                backgroundColor: $r('app.color.app_surface_muted'),
                iconColor: $r('app.color.app_accent_text')
              },
              dividerStyle: {
                dividerColor: $r('sys.color.ohos_id_color_titlebar_bg_transparent')
              }
            }
          },
          scrollEffectStyle: {
            backgroundStyle: {
              backgroundColor: $r('sys.color.ohos_id_color_titlebar_bg'),
              blurRadius: 48,
              maskExtraHeight: 32
            },
            contentStyle: {
              titleStyle: {
                mainTitleColor: $r('sys.color.ohos_id_color_titlebar_text'),
                subTitleColor: $r('sys.color.ohos_id_color_titlebar_subtitle_text')
              },
              backIconStyle: {
                backgroundColor: $r('app.color.app_surface_muted'),
                iconColor: $r('app.color.app_accent_text')
              },
              dividerStyle: {
                dividerColor: $r('sys.color.ohos_id_color_titlebar_bg_transparent')
              }
            }
          },
          systemMaterialEffect: {
            materialType: this.materialType,
            materialLevel: this.materialLevel
          }
        }
      })
      .titleMode(HdsNavDestinationTitleMode.MODAL)
      .hideToolBar(true)
      .onBackPressed(() => this.popDestination())
    } else if (name === 'manage') {
      HdsNavDestination() {
        AppManagePage({
          refreshToken: this.catalogRefreshTick,
          scroller: this.manageScroller,
          onAppsDeleted: (appIdsText: string) => this.handleAppsDeleted(appIdsText)
        })
      }
      .titleBar({
        content: {
          title: {
            mainTitle: '管理',
            mainTitleSize: TitleSize.TITLE_S
          },
          divider: {
            showType: DividerShowType.AUTO
          }
        },
        style: {
          scrollEffectOpts: {
            scrollEffectType: ScrollEffectType.IMMERSIVE_GRADIENT_BLUR
          },
          originalStyle: {
            backgroundStyle: {
              backgroundColor: $r('sys.color.ohos_id_color_titlebar_bg_transparent'),
              blurRadius: 48,
              maskExtraHeight: 32
            },
            contentStyle: {
              titleStyle: {
                mainTitleColor: $r('sys.color.ohos_id_color_titlebar_text'),
                subTitleColor: $r('sys.color.ohos_id_color_titlebar_subtitle_text')
              },
              backIconStyle: {
                backgroundColor: $r('app.color.app_surface_muted'),
                iconColor: $r('app.color.app_accent_text')
              },
              dividerStyle: {
                dividerColor: $r('sys.color.ohos_id_color_titlebar_bg_transparent')
              }
            }
          },
          scrollEffectStyle: {
            backgroundStyle: {
              backgroundColor: $r('sys.color.ohos_id_color_titlebar_bg'),
              blurRadius: 48,
              maskExtraHeight: 32
            },
            contentStyle: {
              titleStyle: {
                mainTitleColor: $r('sys.color.ohos_id_color_titlebar_text'),
                subTitleColor: $r('sys.color.ohos_id_color_titlebar_subtitle_text')
              },
              backIconStyle: {
                backgroundColor: $r('app.color.app_surface_muted'),
                iconColor: $r('app.color.app_accent_text')
              },
              dividerStyle: {
                dividerColor: $r('sys.color.ohos_id_color_titlebar_bg_transparent')
              }
            }
          },
          systemMaterialEffect: {
            materialType: this.materialType,
            materialLevel: this.materialLevel
          }
        }
      })
      .titleMode(HdsNavDestinationTitleMode.MODAL)
      .hideToolBar(true)
      .onBackPressed(() => this.popDestination())
    } else if (name === 'about') {
      HdsNavDestination() {
        AboutPage({
          scroller: this.settingsScroller,
          onOpenPrivacyPolicy: () => this.goToPrivacyPolicy(),
          onOpenGroupPage: () => this.goToAddGroup()
        })
      }
      .titleBar({
        content: {
          title: {
            mainTitle: '关于',
            mainTitleSize: TitleSize.TITLE_S
          },
          divider: {
            showType: DividerShowType.AUTO
          }
        },
        style: {
          scrollEffectOpts: {
            scrollEffectType: ScrollEffectType.IMMERSIVE_GRADIENT_BLUR
          },
          originalStyle: {
            backgroundStyle: {
              backgroundColor: $r('sys.color.ohos_id_color_titlebar_bg_transparent'),
              blurRadius: 48,
              maskExtraHeight: 32
            },
            contentStyle: {
              titleStyle: {
                mainTitleColor: $r('sys.color.ohos_id_color_titlebar_text'),
                subTitleColor: $r('sys.color.ohos_id_color_titlebar_subtitle_text')
              },
              backIconStyle: {
                backgroundColor: $r('app.color.app_surface_muted'),
                iconColor: $r('app.color.app_accent_text')
              },
              dividerStyle: {
                dividerColor: $r('sys.color.ohos_id_color_titlebar_bg_transparent')
              }
            }
          },
          scrollEffectStyle: {
            backgroundStyle: {
              backgroundColor: $r('sys.color.ohos_id_color_titlebar_bg'),
              blurRadius: 48,
              maskExtraHeight: 32
            },
            contentStyle: {
              titleStyle: {
                mainTitleColor: $r('sys.color.ohos_id_color_titlebar_text'),
                subTitleColor: $r('sys.color.ohos_id_color_titlebar_subtitle_text')
              },
              backIconStyle: {
                backgroundColor: $r('app.color.app_surface_muted'),
                iconColor: $r('app.color.app_accent_text')
              },
              dividerStyle: {
                dividerColor: $r('sys.color.ohos_id_color_titlebar_bg_transparent')
              }
            }
          },
          systemMaterialEffect: {
            materialType: this.materialType,
            materialLevel: this.materialLevel
          }
        }
      })
      .titleMode(HdsNavDestinationTitleMode.MODAL)
      .hideToolBar(true)
      .onBackPressed(() => this.popDestination())
    } else if (name === 'addGroup') {
      HdsNavDestination() {
        AddGroupPage()
      }
      .titleBar({
        content: {
          title: {
            mainTitle: '加入群聊',
            mainTitleSize: TitleSize.TITLE_S
          },
          divider: {
            showType: DividerShowType.AUTO
          }
        },
        style: {
          scrollEffectOpts: {
            scrollEffectType: ScrollEffectType.IMMERSIVE_GRADIENT_BLUR
          },
          originalStyle: {
            backgroundStyle: {
              backgroundColor: $r('sys.color.ohos_id_color_titlebar_bg_transparent'),
              blurRadius: 48,
              maskExtraHeight: 32
            },
            contentStyle: {
              titleStyle: {
                mainTitleColor: $r('sys.color.ohos_id_color_titlebar_text'),
                subTitleColor: $r('sys.color.ohos_id_color_titlebar_subtitle_text')
              },
              backIconStyle: {
                backgroundColor: $r('app.color.app_surface_muted'),
                iconColor: $r('app.color.app_accent_text')
              },
              dividerStyle: {
                dividerColor: $r('sys.color.ohos_id_color_titlebar_bg_transparent')
              }
            }
          },
          scrollEffectStyle: {
            backgroundStyle: {
              backgroundColor: $r('sys.color.ohos_id_color_titlebar_bg'),
              blurRadius: 48,
              maskExtraHeight: 32
            },
            contentStyle: {
              titleStyle: {
                mainTitleColor: $r('sys.color.ohos_id_color_titlebar_text'),
                subTitleColor: $r('sys.color.ohos_id_color_titlebar_subtitle_text')
              },
              backIconStyle: {
                backgroundColor: $r('app.color.app_surface_muted'),
                iconColor: $r('app.color.app_accent_text')
              },
              dividerStyle: {
                dividerColor: $r('sys.color.ohos_id_color_titlebar_bg_transparent')
              }
            }
          },
          systemMaterialEffect: {
            materialType: this.materialType,
            materialLevel: this.materialLevel
          }
        }
      })
      .titleMode(HdsNavDestinationTitleMode.MODAL)
      .hideToolBar(true)
      .onBackPressed(() => this.popDestination())
    } else if (name === 'web') {
      HdsNavDestination() {
        WebPage({
          url: this.webPageUrl(params)
        })
      }
      .titleBar({
        content: {
          title: {
            mainTitle: this.webPageTitle(params),
            mainTitleSize: TitleSize.TITLE_S
          },
          divider: {
            showType: DividerShowType.AUTO
          }
        },
        style: {
          scrollEffectOpts: {
            scrollEffectType: ScrollEffectType.IMMERSIVE_GRADIENT_BLUR
          },
          originalStyle: {
            backgroundStyle: {
              backgroundColor: $r('sys.color.ohos_id_color_titlebar_bg_transparent'),
              blurRadius: 48,
              maskExtraHeight: 32
            },
            contentStyle: {
              titleStyle: {
                mainTitleColor: $r('sys.color.ohos_id_color_titlebar_text'),
                subTitleColor: $r('sys.color.ohos_id_color_titlebar_subtitle_text')
              },
              backIconStyle: {
                backgroundColor: $r('app.color.app_surface_muted'),
                iconColor: $r('app.color.app_accent_text')
              },
              dividerStyle: {
                dividerColor: $r('sys.color.ohos_id_color_titlebar_bg_transparent')
              }
            }
          },
          scrollEffectStyle: {
            backgroundStyle: {
              backgroundColor: $r('sys.color.ohos_id_color_titlebar_bg'),
              blurRadius: 48,
              maskExtraHeight: 32
            },
            contentStyle: {
              titleStyle: {
                mainTitleColor: $r('sys.color.ohos_id_color_titlebar_text'),
                subTitleColor: $r('sys.color.ohos_id_color_titlebar_subtitle_text')
              },
              backIconStyle: {
                backgroundColor: $r('app.color.app_surface_muted'),
                iconColor: $r('app.color.app_accent_text')
              },
              dividerStyle: {
                dividerColor: $r('sys.color.ohos_id_color_titlebar_bg_transparent')
              }
            }
          },
          systemMaterialEffect: {
            materialType: this.materialType,
            materialLevel: this.materialLevel
          }
        }
      })
      .titleMode(HdsNavDestinationTitleMode.MODAL)
      .hideToolBar(true)
      .onBackPressed(() => this.popDestination())
    } else if (name === 'run') {
      HdsNavDestination() {
        RunPage({
          appId: this.runAppId(params),
          settingsRevision: this.runtimeSettingsRevision,
          destinationVisible: this.runtimeDestinationVisible,
          destinationVisibilityRevision: this.runtimeDestinationVisibilityRevision,
          onBack: () => this.backToList(),
          onRecentChanged: (appId: string) => this.recordRecentRun(appId)
        })
      }
        .titleBar({
          content: {
            title: {
              mainTitle: '运行',
              subTitle: this.runAppId(params)
            },
            menu: {
              value: this.runMenuItems(this.runAppId(params))
            },
            divider: {
              showType: DividerShowType.AUTO
            }
          },
          style: {
            scrollEffectOpts: {
              scrollEffectType: ScrollEffectType.IMMERSIVE_GRADIENT_BLUR
            },
            originalStyle: {
              backgroundStyle: {
                backgroundColor: $r('sys.color.ohos_id_color_titlebar_bg_transparent'),
                blurRadius: 48,
                maskExtraHeight: 32
              },
              contentStyle: {
                titleStyle: {
                  mainTitleColor: $r('sys.color.ohos_id_color_titlebar_text'),
                  subTitleColor: $r('sys.color.ohos_id_color_titlebar_subtitle_text')
                },
                backIconStyle: {
                  backgroundColor: $r('sys.color.ohos_id_color_button_normal'),
                  iconColor: $r('sys.color.ohos_id_color_titlebar_text')
                },
                dividerStyle: {
                  dividerColor: $r('sys.color.ohos_id_color_titlebar_bg_transparent')
                },
                menuStyle: {
                  backgroundColor: $r('sys.color.ohos_id_color_button_normal'),
                  iconColor: $r('sys.color.ohos_id_color_titlebar_text'),
                  textColor: $r('sys.color.ohos_id_color_titlebar_text')
                }
              }
            },
            scrollEffectStyle: {
              backgroundStyle: {
                backgroundColor: $r('sys.color.ohos_id_color_titlebar_bg'),
                blurRadius: 48,
                maskExtraHeight: 32
              },
              contentStyle: {
                titleStyle: {
                  mainTitleColor: $r('sys.color.ohos_id_color_titlebar_text'),
                  subTitleColor: $r('sys.color.ohos_id_color_titlebar_subtitle_text')
                },
                backIconStyle: {
                  backgroundColor: $r('sys.color.ohos_id_color_button_normal'),
                  iconColor: $r('sys.color.ohos_id_color_titlebar_text')
                },
                dividerStyle: {
                  dividerColor: $r('sys.color.ohos_id_color_titlebar_bg_transparent')
                },
                menuStyle: {
                  backgroundColor: $r('sys.color.ohos_id_color_button_normal'),
                  iconColor: $r('sys.color.ohos_id_color_titlebar_text'),
                  textColor: $r('sys.color.ohos_id_color_titlebar_text')
                }
              }
            },
            systemMaterialEffect: {
              materialType: this.materialType,
              materialLevel: this.materialLevel
            }
          }
        })
        .titleMode(HdsNavDestinationTitleMode.MINI)
        .hideToolBar(true)
        .onShown(() => this.handleRunDestinationShown())
        .onHidden(() => this.handleRunDestinationHidden())
        .onBackPressed(() => this.backFromRun())
    } else if (name === 'runSettings') {
      HdsNavDestination() {
        SettingsPage({
          scroller: this.runSettingsScroller,
          appId: this.runSettingsAppId(params),
          appTitle: this.runSettingsAppId(params),
          onSettingsChanged: () => {
            this.runtimeSettingsRevision += 1
          }
        })
      }
      .titleBar({
        content: {
          title: {
            mainTitle: '游戏设置',
            subTitle: this.runSettingsAppId(params),
            mainTitleSize: TitleSize.TITLE_S
          },
          divider: {
            showType: DividerShowType.AUTO
          }
        },
        style: {
          scrollEffectOpts: {
            scrollEffectType: ScrollEffectType.IMMERSIVE_GRADIENT_BLUR
          },
          originalStyle: {
            backgroundStyle: {
              backgroundColor: $r('sys.color.ohos_id_color_titlebar_bg_transparent'),
              blurRadius: 48,
              maskExtraHeight: 32
            },
            contentStyle: {
              titleStyle: {
                mainTitleColor: $r('sys.color.ohos_id_color_titlebar_text'),
                subTitleColor: $r('sys.color.ohos_id_color_titlebar_subtitle_text')
              },
              backIconStyle: {
                backgroundColor: $r('sys.color.ohos_id_color_button_normal'),
                iconColor: $r('sys.color.ohos_id_color_titlebar_text')
              },
              dividerStyle: {
                dividerColor: $r('sys.color.ohos_id_color_titlebar_bg_transparent')
              }
            }
          },
          scrollEffectStyle: {
            backgroundStyle: {
              backgroundColor: $r('sys.color.ohos_id_color_titlebar_bg'),
              blurRadius: 48,
              maskExtraHeight: 32
            },
            contentStyle: {
              titleStyle: {
                mainTitleColor: $r('sys.color.ohos_id_color_titlebar_text'),
                subTitleColor: $r('sys.color.ohos_id_color_titlebar_subtitle_text')
              },
              backIconStyle: {
                backgroundColor: $r('sys.color.ohos_id_color_button_normal'),
                iconColor: $r('sys.color.ohos_id_color_titlebar_text')
              },
              dividerStyle: {
                dividerColor: $r('sys.color.ohos_id_color_titlebar_bg_transparent')
              }
            }
          },
          systemMaterialEffect: {
            materialType: this.materialType,
            materialLevel: this.materialLevel
          }
        }
      })
      .titleMode(HdsNavDestinationTitleMode.MODAL)
      .hideToolBar(true)
      .onBackPressed(() => this.backFromRunSettings())
    }
  }

  build() {
    Stack() {
      Column()
        .width('100%')
        .height('100%')
        .linearGradient({
          angle: 180,
          colors: [
            [$r('app.color.home_background_start'), 0.0],
            [$r('app.color.home_background_mid'), 0.48],
            [$r('app.color.home_background_end'), 1.0]
          ]
        })
        .expandSafeArea([SafeAreaType.SYSTEM], [SafeAreaEdge.TOP, SafeAreaEdge.BOTTOM])

      Column() {
        HdsNavigation(this.pathStack) {
          Column() {
            Text(`${this.catalogRefreshTick}-${this.recentRefreshTick}-${this.selectedTabIndex}-${this.recentAppIdsText}`).fontSize(1).opacity(0)
            this.HomeTabsBuilder()
          }
          .width('100%')
          .height('100%')
        }
        .width('100%')
        .layoutWeight(1)
        .titleBar({
          content: {
            title: {
              mainTitle: this.rootTitle()
            },
            menu: {
              value: this.rootMenuItems()
            },
            divider: {
              showType: DividerShowType.AUTO
            }
          },
          style: {
            scrollEffectOpts: {
              scrollEffectType: ScrollEffectType.IMMERSIVE_GRADIENT_BLUR
            },
            originalStyle: {
              backgroundStyle: {
                backgroundColor: $r('sys.color.ohos_id_color_titlebar_bg_transparent'),
                blurRadius: 48,
                maskExtraHeight: 32
              },
              contentStyle: {
              titleStyle: {
                mainTitleColor: $r('sys.color.ohos_id_color_titlebar_text'),
                subTitleColor: $r('sys.color.ohos_id_color_titlebar_subtitle_text')
              },
              dividerStyle: {
                dividerColor: $r('sys.color.ohos_id_color_titlebar_bg_transparent')
              },
              menuStyle: {
                backgroundColor: $r('app.color.app_surface_muted'),
                iconColor: $r('app.color.app_accent_text'),
                textColor: $r('app.color.app_accent_text')
              }
            }
          },
            scrollEffectStyle: {
              backgroundStyle: {
                backgroundColor: $r('sys.color.ohos_id_color_titlebar_bg'),
                blurRadius: 48,
                maskExtraHeight: 32
              },
              contentStyle: {
              titleStyle: {
                mainTitleColor: $r('sys.color.ohos_id_color_titlebar_text'),
                subTitleColor: $r('sys.color.ohos_id_color_titlebar_subtitle_text')
              },
              dividerStyle: {
                dividerColor: $r('sys.color.ohos_id_color_titlebar_bg_transparent')
              },
              menuStyle: {
                backgroundColor: $r('app.color.app_surface_muted'),
                iconColor: $r('app.color.app_accent_text'),
                textColor: $r('app.color.app_accent_text')
              }
            }
          },
            systemMaterialEffect: {
              materialType: this.materialType,
              materialLevel: this.materialLevel
            }
          }
        })
        .bindToScrollable([this.activeRootScroller()])
        .titleMode(HdsNavigationTitleMode.MINI)
        .hideToolBar(true)
        .hideBackButton(true)
        .navDestination(this.DestinationBuilder)
        .padding({
          top: this.navigationTopSafeGap
        })
      }
      .width('100%')
      .height('100%')

    }
    .width('100%')
    .height('100%')
  }
}
