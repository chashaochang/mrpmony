import type { AppItemVM } from '../../models/AppItemVM'

@Component
export struct AppListView {
  @Prop items: AppItemVM[] = []
  onItemTap?: (appId: string) => void

  build() {
    List() {
      ForEach(this.items, (item: AppItemVM) => {
        ListItem() {
          Column() {
            Text(item.name).fontSize(18)
            Text(item.version || '').fontSize(12)
            Text(item.status === 'ready' ? '可运行' : '不可运行').fontSize(12)
          }
          .width('100%')
          .padding(16)
          .opacity(item.status === 'ready' ? 1 : 0.5)
          .alignItems(HorizontalAlign.Start)
        }
        .onClick(() => {
          if (item.status !== 'ready') {
            return
          }
          if (this.onItemTap) {
            this.onItemTap(item.id)
          }
        })
      }, (item: AppItemVM) => item.id)
    }
  }
}
