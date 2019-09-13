import WinSDK
import SwiftWin32
import CPlaygroundSupport

typealias EnumChildProc = @convention(c) (HWND, LPARAM) -> Bool
let DestroyChildrenProc: WNDENUMPROC = { (hWnd, lParam) in
    return WindowsBool(DestroyWindow(hWnd))
}

public final class PlaygroundPage {
    public static let current: PlaygroundPage = PlaygroundPage()

    public var liveView: View? = nil {
        didSet {
            // Don't do anything if we just went from nil to nil.
            guard !(liveView == nil && oldValue == nil) else { return }

            if let old = oldValue {
                EnumChildWindows(old._handle, DestroyChildrenProc, 0)
            }
            if let liveView = liveView {
                let handle = liveView._handle
                let style = GetWindowLongA(handle, GWL_STYLE)
                SetWindowLongA(handle, GWL_STYLE, (style | WS_CHILD))
                let styleEx = GetWindowLongA(handle, GWL_EXSTYLE)
                SetWindowLongA(handle, GWL_EXSTYLE, styleEx | WS_EX_MDICHILD)
                SetParent(handle, g_ui)
                var rect = RECT()
                GetClientRect(g_ui, &rect)
                SetWindowPos(handle, unsafeBitCast(1, to: HWND?.self),
                             rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                             UINT(bitPattern: SWP_NOZORDER));
                liveView.show()
                liveView.invalidate();
            }
        }
    }
}
