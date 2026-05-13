import { AppService } from '../services/AppService'
import type { AppItemVM } from '../models/AppItemVM'
import type { UiError } from '../models/UiError'
import { StoreObserver } from './StoreObserver'

function errorToString(error: Object): string {
  if (error instanceof Error) {
    return error.message
  }
  return JSON.stringify(error)
}

export class AppCatalogStore extends StoreObserver {
  private static sharedStore?: AppCatalogStore
  items: AppItemVM[] = []
  loading: boolean = false
  error: UiError | null = null
  private activeLoadToken: number = 0
  private loadPromise: Promise<void> | null = null

  constructor(private readonly service: AppService = new AppService()) {
    super()
  }

  static shared(): AppCatalogStore {
    if (!AppCatalogStore.sharedStore) {
      AppCatalogStore.sharedStore = new AppCatalogStore()
    }
    return AppCatalogStore.sharedStore
  }

  async loadApps(): Promise<void> {
    if (this.loadPromise) {
      await this.loadPromise
      return
    }
    const token = this.activeLoadToken + 1
    this.activeLoadToken = token
    const promise = this.performLoadApps(token)
    this.loadPromise = promise
    try {
      await promise
    } finally {
      if (this.loadPromise === promise) {
        this.loadPromise = null
      }
    }
  }

  private async performLoadApps(token: number): Promise<void> {
    this.loading = true
    this.error = null
    this.notify()
    try {
      const nextItems = await this.service.listApps()
      if (token === this.activeLoadToken) {
        this.items = nextItems
      }
    } catch (_error) {
      if (token === this.activeLoadToken) {
        this.error = { message: '应用列表加载失败', retryable: true }
      }
    } finally {
      if (token === this.activeLoadToken) {
        this.loading = false
        this.notify()
      }
    }
  }

  async refreshApps(): Promise<void> {
    await this.loadApps()
  }

  async deleteApp(appId: string, deleteRelatedData: boolean = false): Promise<boolean> {
    this.activeLoadToken += 1
    this.loadPromise = null
    this.loading = false
    this.error = null
    this.notify()
    try {
      await this.service.deleteApp(appId, deleteRelatedData)
      this.items = await this.service.listApps()
      this.notify()
      return true
    } catch (error) {
      const message = errorToString(error as Object)
      this.error = { message: `删除 MRP 失败：${message}`, retryable: false }
      this.notify()
      return false
    }
  }

  async deleteApps(appIds: string[], deleteRelatedData: boolean = false): Promise<string[]> {
    this.activeLoadToken += 1
    this.loadPromise = null
    this.loading = false
    this.error = null
    this.notify()
    const deletedIds: string[] = []
    try {
      for (const appId of appIds) {
        await this.service.deleteApp(appId, deleteRelatedData)
        deletedIds.push(appId)
      }
      this.items = await this.service.listApps()
      this.notify()
      return deletedIds
    } catch (error) {
      try {
        this.items = await this.service.listApps()
      } catch (_refreshError) {
        if (deletedIds.length > 0) {
          const deletedMap: Record<string, boolean> = {}
          for (const appId of deletedIds) {
            deletedMap[appId] = true
          }
          this.items = this.items.filter((item: AppItemVM) => !deletedMap[item.id])
        }
      }
      const message = errorToString(error as Object)
      this.error = {
        message: deletedIds.length > 0 ? `部分 MRP 删除失败：${message}` : `删除 MRP 失败：${message}`,
        retryable: false
      }
      this.notify()
      return deletedIds
    }
  }

  clearError(): void {
    this.error = null
    this.notify()
  }
}
