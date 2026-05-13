import { NativeAppAdapter } from '../adapters/NativeAppAdapter'
import type { AppItemKind, AppItemVM } from '../models/AppItemVM'
import { AppPreferences } from './AppPreferences'

export class AppService {
  private readonly appPreferences: AppPreferences = new AppPreferences()

  constructor(private readonly adapter: NativeAppAdapter = new NativeAppAdapter()) {}

  async listApps(): Promise<AppItemVM[]> {
    const apps = await this.adapter.listInstalledApps()
    return apps.map((item) => ({
      id: item.appId,
      name: item.name,
      fileName: item.fileName,
      description: item.description,
      icon: item.icon,
      version: item.version,
      addedAt: item.addedAt,
      sizeBytes: item.sizeBytes,
      kind: this.resolveKind(item.name, item.fileName, item.description),
      status: item.runnable ? 'ready' : 'invalid'
    }))
  }

  async deleteApp(appId: string, deleteRelatedData: boolean = false): Promise<void> {
    await this.adapter.deleteInstalledApp(appId, { deleteRelatedData })
    if (deleteRelatedData) {
      await this.appPreferences.deleteEmulatorSettingsForApp(appId)
    }
  }

  private resolveKind(name: string, fileName: string, description?: string): AppItemKind {
    const title = `${name} ${fileName}`.toLowerCase()
    const text = `${title} ${description ?? ''}`.toLowerCase()
    const gameKeywords: string[] = [
      '游戏',
      '小游戏',
      '动作',
      '射击',
      '格斗',
      '塔防',
      '棋牌',
      '麻将',
      '斗地主',
      '象棋',
      '赛车',
      '飞行',
      '冒险',
      '角色扮演',
      'rpg',
      '关卡',
      '技能',
      '战斗',
      '魔王',
      '怪物',
      '僵尸',
      '忍者',
      '坦克',
      '魂斗罗',
      '魔塔',
      '找茬',
      '打地鼠',
      '祖玛',
      '勇士',
      '勇闯',
      '拳皇',
      '街头霸王',
      '红白机',
      '经典回归'
    ]
    if (this.containsAny(text, gameKeywords)) {
      return 'game'
    }

    const appTitleKeywords: string[] = [
      '12530',
      'qq',
      'msn',
      'app',
      'client',
      'reader',
      'player',
      'tool',
      'utility',
      'edit',
      'editor',
      'calc',
      'calculator',
      '应用列表',
      '应用软件',
      '计算器',
      '记事本',
      '浏览器',
      '字典',
      '词典',
      '指南针',
      '股票',
      '黄历',
      '音乐客户端',
      '播放器',
      '编辑',
      '编程手册',
      '移动视线',
      '短信',
      '祝福',
      '笑话',
      '幽默',
      '绕口令',
      '谜语',
      '食物相克',
      '恋爱教科书'
    ]
    if (this.containsAny(title, appTitleKeywords)) {
      return 'app'
    }

    const appDescriptionKeywords: string[] = [
      '聊天工具',
      '实用小工具',
      '编辑器',
      '编辑助手',
      '播放器',
      '歌词同步',
      '股票行情',
      '动态菜单',
      '开发实验系统',
      '移动设备上阅读',
      '休闲应用'
    ]
    if (this.containsAny((description ?? '').toLowerCase(), appDescriptionKeywords)) {
      return 'app'
    }

    return 'game'
  }

  private containsAny(source: string, keywords: string[]): boolean {
    for (const keyword of keywords) {
      if (source.includes(keyword)) {
        return true
      }
    }
    return false
  }
}
