// Copyright (C) 2013-2017 Michael Kazakov. Subject to GNU General Public License version 3.
#pragma once

#include "PanelViewDelegate.h"
#include <NimbleCommander/Core/rapidjson_fwd.h>
#include <VFS/VFS.h>

struct VFSInstancePromise;
class NetworkConnectionsManager;
@class PanelController;
@class PanelView;
@class BriefSystemOverview;
@class MainWindowFilePanelState;
@class MainWindowController;

namespace nc::panel {
class History;
struct PersistentLocation;

namespace data {
    struct SortMode;
    struct HardFilter;
    struct Model;
}

struct ControllerStateEncoding
{
    enum Options {
        EncodeDataOptions   =  1,
        EncodeViewOptions   =  2,
        EncodeContentState  =  4,
        
        EncodeNothing       =  0,
        EncodeEverything    = -1
    };
};

class ActivityTicket
{
public:
    ActivityTicket();
    ActivityTicket(PanelController *_panel, uint64_t _ticket);
    ActivityTicket(const ActivityTicket&) = delete;
    ActivityTicket(ActivityTicket&&);
    ~ActivityTicket();
    void operator=(const ActivityTicket&) = delete;
    void operator=(ActivityTicket&&);
    
private:
    void Reset();
    uint64_t                ticket;
    __weak PanelController *panel;
};

struct DelayedFocusing
{
    string          filename;
    milliseconds    timeout = 500ms;
    bool            check_now = true;

    /**
     * called by PanelController when succesfully changed the cursor position regarding this request.
     */
    function<void()> done;
};

class DirectoryChangeRequest
{
public:
    /* required */
    string              RequestedDirectory      = "";
    shared_ptr<VFSHost> VFS                     = nullptr;
    
    /* optional */
    string              RequestFocusedEntry     = "";
    bool                PerformAsynchronous     = true;
    bool                LoadPreviousViewState   = false;
    
    /**
     * This will be called from a thread which is loading a vfs listing with
     * vfs result code.
     * This thread may be main or background depending on PerformAsynchronous.
     * Will be called on any error canceling process or with 0 on successful loading.
     */
    function<void(int)> LoadingResultCallback    = nullptr;
    
    /**
     * Return code of a VFS->FetchDirectoryListing will be placed here.
     */
    int                 LoadingResultCode        = 0;
};

}

/**
 * PanelController is reponder to enable menu events processing
 */
@interface PanelController : NSResponder<PanelViewDelegate>

@property (nonatomic) MainWindowFilePanelState* state;
@property (nonatomic, readonly) MainWindowController* mainWindowController;
@property (nonatomic, readonly) PanelView* view;
@property (nonatomic, readonly) const nc::panel::data::Model& data;
@property (nonatomic, readonly) nc::panel::History& history;
@property (nonatomic, readonly) bool isActive;
@property (nonatomic, readonly) bool isUniform; // return true if panel's listing has common vfs host and directory for it's items
@property (nonatomic, readonly) NSWindow* window;
@property (nonatomic, readonly) bool receivesUpdateNotifications; // returns true if underlying vfs will notify controller that content has changed
@property (nonatomic, readonly) bool ignoreDirectoriesOnSelectionByMask;
@property (nonatomic, readonly) int vfsFetchingFlags;
@property (nonatomic) int layoutIndex;
@property (nonatomic, readonly) NetworkConnectionsManager& networkConnectionsManager;

- (optional<rapidjson::StandaloneValue>) encodeRestorableState;
- (bool) loadRestorableState:(const rapidjson::StandaloneValue&)_state;
- (optional<rapidjson::StandaloneValue>) encodeStateWithOptions:(nc::panel::ControllerStateEncoding::Options)_options;

- (void) refreshPanel; // reload panel contents
- (void) forceRefreshPanel; // user pressed cmd+r by default
- (void) markRestorableStateAsInvalid; // will actually call window controller's invalidateRestorableState

- (void) commitCancelableLoadingTask:(function<void(const function<bool()> &_is_cancelled)>) _task;


/**
 * Will copy view options and sorting options.
 */
- (void) copyOptionsFromController:(PanelController*)_pc;

/**
 * RAII principle - when ActivityTicket dies - it will clear activity flag.
 * Thread-safe.
 */
- (nc::panel::ActivityTicket) registerExtActivity;


// panel sorting settings
- (void) changeSortingModeTo:(nc::panel::data::SortMode)_mode;
- (void) changeHardFilteringTo:(nc::panel::data::HardFilter)_filter;

// PanelView callback hooks
- (void) panelViewDidBecomeFirstResponder;
- (void) panelViewDidChangePresentationLayout;

// managing entries selection
- (void) selectEntriesWithFilenames:(const vector<string>&)_filenames;
- (void) setEntriesSelection:(const vector<bool>&)_selection;


- (void) calculateSizesOfItems:(const vector<VFSListingItem>&)_items;


- (int) GoToDirWithContext:(shared_ptr<nc::panel::DirectoryChangeRequest>)_context;


// will not load previous view state if any
// don't use the following methds. use GoToDirWithContext instead.
- (int) GoToDir:(const string&)_dir
            vfs:(shared_ptr<VFSHost>)_vfs
   select_entry:(const string&)_filename
          async:(bool)_asynchronous;

- (int) GoToDir:(const string&)_dir
            vfs:(shared_ptr<VFSHost>)_vfs
   select_entry:(const string&)_filename
loadPreviousState:(bool)_load_state
          async:(bool)_asynchronous;

// sync operation
- (void) loadNonUniformListing:(const shared_ptr<VFSListing>&)_listing;

// will load previous view state if any
- (void) GoToVFSPromise:(const VFSInstancePromise&)_promise onPath:(const string&)_directory;
// some params later

- (void) goToPersistentLocation:(const nc::panel::PersistentLocation &)_location;

- (void) RecoverFromInvalidDirectory;

/** 
 * Delayed entry selection change - panel controller will memorize such request.
 * If _check_now flag is on then controller will look for requested element and if it was found - select it.
 * If there was another pending selection request - it will be overwrited by the new one.
 * Controller will check for entry appearance on every directory update.
 * Request will be removed upon directory change.
 * Once request is accomplished it will be removed.
 * If on any checking it will be found that time for request has went out - it will be removed (500ms is just ok for _time_out_in_ms).
 * Will also deselect any currenly selected items.
 */
- (void) scheduleDelayedFocusing:(nc::panel::DelayedFocusing)request;

- (void) clearQuickSearchFiltering;
- (void) QuickSearchSetCriteria:(NSString *)_text;

- (void) requestQuickRenamingOfItem:(VFSListingItem)_item to:(const string&)_new_filename;

- (void)updateAttachedQuickLook;
- (void)updateAttachedBriefSystemOverview;
@end

// internal stuff, move it somewehere else
@interface PanelController ()
- (void) finishExtActivityWithTicket:(uint64_t)_ticket;
- (void) CancelBackgroundOperations;
- (void) contextMenuDidClose:(NSMenu*)_menu;
@end

#import "PanelController+DataAccess.h"