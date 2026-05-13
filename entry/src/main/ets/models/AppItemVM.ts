export type AppItemKind = 'game' | 'app'

export interface AppItemVM {
  id: string
  name: string
  fileName: string
  description?: string
  icon?: string
  version?: string
  addedAt?: number
  sizeBytes?: number
  kind: AppItemKind
  status: 'ready' | 'invalid'
}
