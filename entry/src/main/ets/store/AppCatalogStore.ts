import { AppService } from '../services/AppService'
import type { AppItemVM } from '../models/AppItemVM'
import type { UiError } from '../models/UiError'
import { StoreObserver } from './StoreObserver'

export class AppCatalogStore extends StoreObserver {
  items: AppItemVM[] = []
  loading: boolean = false
  error: UiError | null = null

  constructor(private readonly service: AppService = new AppService()) {
    super()
  }

  async loadApps(): Promise<void> {
    this.loading = true
    this.error = null
    this.notify()
    try {
      this.items = await this.service.listApps()
    } catch (_error) {
      this.error = { message: '应用列表加载失败', retryable: true }
    } finally {
      this.loading = false
      this.notify()
    }
  }

  async refreshApps(): Promise<void> {
    await this.loadApps()
  }

  clearError(): void {
    this.error = null
    this.notify()
  }
}
