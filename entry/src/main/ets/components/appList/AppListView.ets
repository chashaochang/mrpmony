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
            Text(item.status).fontSize(12)
          }
          .width('100%')
          .padding(16)
          .alignItems(HorizontalAlign.Start)
        }
        .onClick(() => {
          if (this.onItemTap) {
            this.onItemTap(item.id)
          }
        })
      }, (item: AppItemVM) => item.id)
    }
  }
}
