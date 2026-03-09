import { AppService } from '../services/AppService'
import type { AppItemVM } from '../models/AppItemVM'
import type { UiError } from '../models/UiError'

export class AppCatalogStore {
  items: AppItemVM[] = []
  loading: boolean = false
  error: UiError | null = null

  constructor(private readonly service: AppService = new AppService()) {}

  async loadApps(): Promise<void> {
    this.loading = true
    this.error = null
    try {
      this.items = await this.service.listApps()
    } catch (_error) {
      this.error = { message: '应用列表加载失败', retryable: true }
    } finally {
      this.loading = false
    }
  }

  async refreshApps(): Promise<void> {
    await this.loadApps()
  }

  clearError(): void {
    this.error = null
  }
}
