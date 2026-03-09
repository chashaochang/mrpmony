import type { AppItemVM } from '../../models/AppItemVM'

@Component
export struct AppListView {
  @Prop items: AppItemVM[] = []

  build() {
    List() {
      ForEach(this.items, (item: AppItemVM) => {
        ListItem() {
          Text(item.name)
        }
      }, (item: AppItemVM) => item.id)
    }
  }
}
