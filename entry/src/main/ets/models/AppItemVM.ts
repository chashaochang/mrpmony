export interface AppItemVM {
  id: string
  name: string
  icon?: string
  version?: string
  status: 'ready' | 'invalid'
}
