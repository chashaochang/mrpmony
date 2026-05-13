import common from '@ohos.app.ability.common'
import preferences from '@ohos.data.preferences'

export type KeypadMode = 'classic' | 'compact' | 'touch'
export type ListSortMode = 'name' | 'added' | 'recent'

export interface EmulatorSettings {
  keypadMode: KeypadMode
  scalePercent: number
  showFrameStats: boolean
}

const PREFERENCES_NAME = 'mrp_emulator_preferences'
const KEY_RECENT_APP_IDS = 'recentAppIdsText'
const KEY_LIST_SORT_MODE = 'listSortMode'
const KEY_KEYPAD_MODE = 'keypadMode'
const KEY_SCALE_PERCENT = 'scalePercent'
const KEY_SHOW_FRAME_STATS = 'showFrameStats'
const KEY_APP_SETTINGS_PREFIX = 'appSettings'
const KEY_APP_SETTINGS_ENABLED = 'enabled'

const DEFAULT_SETTINGS: EmulatorSettings = {
  keypadMode: 'classic',
  scalePercent: 100,
  showFrameStats: false
}

export function defaultEmulatorSettings(): EmulatorSettings {
  return {
    keypadMode: DEFAULT_SETTINGS.keypadMode,
    scalePercent: DEFAULT_SETTINGS.scalePercent,
    showFrameStats: DEFAULT_SETTINGS.showFrameStats
  }
}

let abilityContext: common.UIAbilityContext | undefined
let preferenceStore: preferences.Preferences | undefined
let preferenceStorePromise: Promise<preferences.Preferences> | undefined
let cachedGlobalSettings: EmulatorSettings | undefined
let cachedAppSettings: Record<string, EmulatorSettings> = {}
let cachedAppSettingsEnabled: Record<string, boolean> = {}
let cachedRecentAppIdsText: string | undefined
let cachedListSortMode: ListSortMode | undefined

export function configureAppPreferences(context: common.UIAbilityContext): void {
  abilityContext = context
  preferenceStore = undefined
  preferenceStorePromise = undefined
  cachedGlobalSettings = undefined
  cachedAppSettings = {}
  cachedAppSettingsEnabled = {}
  cachedRecentAppIdsText = undefined
  cachedListSortMode = undefined
}

export class AppPreferences {
  recentAppIdsText(): string {
    if (cachedRecentAppIdsText !== undefined) {
      return cachedRecentAppIdsText
    }
    return this.getString(KEY_RECENT_APP_IDS, '')
  }

  saveRecentAppIdsText(value: string): void {
    cachedRecentAppIdsText = value
    this.putStringAsync(KEY_RECENT_APP_IDS, value)
  }

  listSortMode(): ListSortMode {
    if (cachedListSortMode !== undefined) {
      return cachedListSortMode
    }
    const value = this.getString(KEY_LIST_SORT_MODE, 'name')
    if (value === 'added' || value === 'recent') {
      cachedListSortMode = value
      return value
    }
    cachedListSortMode = 'name'
    return 'name'
  }

  saveListSortMode(value: ListSortMode): void {
    cachedListSortMode = value
    this.putStringAsync(KEY_LIST_SORT_MODE, value)
  }

  emulatorSettings(): EmulatorSettings {
    if (cachedGlobalSettings) {
      return this.copySettings(cachedGlobalSettings)
    }
    return this.globalEmulatorSettings()
  }

  emulatorSettingsForApp(appId?: string): EmulatorSettings {
    if (!appId || appId.length === 0) {
      return this.emulatorSettings()
    }
    const scope = this.appSettingsScope(appId)
    const cachedEnabled = cachedAppSettingsEnabled[scope]
    if (cachedEnabled === false) {
      return this.emulatorSettings()
    }
    if (cachedEnabled === true && cachedAppSettings[scope]) {
      return this.copySettings(cachedAppSettings[scope])
    }
    const globalSettings = this.emulatorSettings()
    const enabled = this.getBoolean(this.appSettingsKey(scope, KEY_APP_SETTINGS_ENABLED), false)
    if (!enabled) {
      return globalSettings
    }
    return {
      keypadMode: this.keypadMode(this.appSettingsKey(scope, KEY_KEYPAD_MODE), globalSettings.keypadMode),
      scalePercent: this.scalePercent(this.appSettingsKey(scope, KEY_SCALE_PERCENT), globalSettings.scalePercent),
      showFrameStats: this.getBoolean(this.appSettingsKey(scope, KEY_SHOW_FRAME_STATS), globalSettings.showFrameStats)
    }
  }

  private globalEmulatorSettings(): EmulatorSettings {
    return {
      keypadMode: this.keypadMode(KEY_KEYPAD_MODE, DEFAULT_SETTINGS.keypadMode),
      scalePercent: this.scalePercent(KEY_SCALE_PERCENT, DEFAULT_SETTINGS.scalePercent),
      showFrameStats: this.getBoolean(KEY_SHOW_FRAME_STATS, DEFAULT_SETTINGS.showFrameStats)
    }
  }

  saveEmulatorSettings(settings: EmulatorSettings): void {
    cachedGlobalSettings = this.copySettings(settings)
    this.saveEmulatorSettingsAsync(settings)
  }

  saveEmulatorSettingsForApp(appId: string, settings: EmulatorSettings): void {
    const scope = this.appSettingsScope(appId)
    cachedAppSettingsEnabled[scope] = true
    cachedAppSettings[scope] = this.copySettings(settings)
    this.saveAppEmulatorSettingsAsync(scope, settings)
  }

  resetEmulatorSettings(): void {
    this.saveEmulatorSettings(defaultEmulatorSettings())
  }

  resetEmulatorSettingsForApp(appId: string): void {
    const scope = this.appSettingsScope(appId)
    cachedAppSettingsEnabled[scope] = false
    this.saveAppSettingsEnabledAsync(scope, false)
  }

  async deleteEmulatorSettingsForApp(appId: string): Promise<void> {
    if (!appId || appId.length === 0) {
      return
    }
    const scope = this.appSettingsScope(appId)
    delete cachedAppSettings[scope]
    delete cachedAppSettingsEnabled[scope]
    const store = await this.storeAsync()
    if (!store) {
      return
    }
    await Promise.all([
      store.delete(this.appSettingsKey(scope, KEY_APP_SETTINGS_ENABLED)),
      store.delete(this.appSettingsKey(scope, KEY_KEYPAD_MODE)),
      store.delete(this.appSettingsKey(scope, KEY_SCALE_PERCENT)),
      store.delete(this.appSettingsKey(scope, KEY_SHOW_FRAME_STATS))
    ])
    await store.flush()
  }

  private keypadMode(key: string, fallback: KeypadMode): KeypadMode {
    const value = this.getString(key, fallback)
    if (value === 'compact' || value === 'touch') {
      return value
    }
    return 'classic'
  }

  private scalePercent(key: string, fallback: number): number {
    const value = this.getNumber(key, fallback)
    if (value < 80) {
      return 80
    }
    if (value > 140) {
      return 140
    }
    return Math.round(value)
  }

  private appSettingsScope(appId: string): string {
    return appId.replace(/[^A-Za-z0-9_.-]/g, '_')
  }

  private appSettingsKey(scope: string, name: string): string {
    return `${KEY_APP_SETTINGS_PREFIX}.${scope}.${name}`
  }

  private copySettings(settings: EmulatorSettings): EmulatorSettings {
    return {
      keypadMode: settings.keypadMode,
      scalePercent: settings.scalePercent,
      showFrameStats: settings.showFrameStats
    }
  }

  private getString(key: string, fallback: string): string {
    const store = this.store()
    if (!store) {
      return fallback
    }
    try {
      const value = store.getSync(key, fallback)
      return typeof value === 'string' ? value : fallback
    } catch (_error) {
      return fallback
    }
  }

  private getNumber(key: string, fallback: number): number {
    const store = this.store()
    if (!store) {
      return fallback
    }
    try {
      const value = store.getSync(key, fallback)
      return typeof value === 'number' ? value : fallback
    } catch (_error) {
      return fallback
    }
  }

  private getBoolean(key: string, fallback: boolean): boolean {
    const store = this.store()
    if (!store) {
      return fallback
    }
    try {
      const value = store.getSync(key, fallback)
      return typeof value === 'boolean' ? value : fallback
    } catch (_error) {
      return fallback
    }
  }

  private putStringAsync(key: string, value: string): void {
    this.storeAsync()
      .then((store: preferences.Preferences | undefined) => {
        if (!store) {
          return
        }
        return store.put(key, value)
          .then(() => store.flush())
      })
      .catch((_error: Error) => {
      })
  }

  private saveEmulatorSettingsAsync(settings: EmulatorSettings): void {
    this.storeAsync()
      .then((store: preferences.Preferences | undefined) => {
        if (!store) {
          return
        }
        return Promise.all([
          store.put(KEY_KEYPAD_MODE, settings.keypadMode),
          store.put(KEY_SCALE_PERCENT, settings.scalePercent),
          store.put(KEY_SHOW_FRAME_STATS, settings.showFrameStats)
        ]).then(() => store.flush())
      })
      .catch((_error: Error) => {
      })
  }

  private saveAppEmulatorSettingsAsync(scope: string, settings: EmulatorSettings): void {
    this.storeAsync()
      .then((store: preferences.Preferences | undefined) => {
        if (!store) {
          return
        }
        return Promise.all([
          store.put(this.appSettingsKey(scope, KEY_APP_SETTINGS_ENABLED), true),
          store.put(this.appSettingsKey(scope, KEY_KEYPAD_MODE), settings.keypadMode),
          store.put(this.appSettingsKey(scope, KEY_SCALE_PERCENT), settings.scalePercent),
          store.put(this.appSettingsKey(scope, KEY_SHOW_FRAME_STATS), settings.showFrameStats)
        ]).then(() => store.flush())
      })
      .catch((_error: Error) => {
      })
  }

  private saveAppSettingsEnabledAsync(scope: string, enabled: boolean): void {
    this.storeAsync()
      .then((store: preferences.Preferences | undefined) => {
        if (!store) {
          return
        }
        return store.put(this.appSettingsKey(scope, KEY_APP_SETTINGS_ENABLED), enabled)
          .then(() => store.flush())
      })
      .catch((_error: Error) => {
      })
  }

  private async storeAsync(): Promise<preferences.Preferences | undefined> {
    if (preferenceStore) {
      return preferenceStore
    }
    if (!abilityContext) {
      return undefined
    }
    try {
      if (!preferenceStorePromise) {
        preferenceStorePromise = preferences.getPreferences(abilityContext, { name: PREFERENCES_NAME })
      }
      preferenceStore = await preferenceStorePromise
      return preferenceStore
    } catch (_error) {
      preferenceStorePromise = undefined
      return undefined
    }
  }

  private store(): preferences.Preferences | undefined {
    if (preferenceStore) {
      return preferenceStore
    }
    if (!abilityContext) {
      return undefined
    }
    try {
      preferenceStore = preferences.getPreferencesSync(abilityContext, { name: PREFERENCES_NAME })
      return preferenceStore
    } catch (_error) {
      return undefined
    }
  }

}
