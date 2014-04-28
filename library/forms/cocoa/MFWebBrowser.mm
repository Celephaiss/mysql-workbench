/* 
 * Copyright (c) 2010, 2013, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#import "MFWebBrowser.h"
#import "MFMForms.h"

@implementation MFWebBrowserImpl

//--------------------------------------------------------------------------------------------------

- (id)initWithObject: (mforms::WebBrowser*) aBrowser
{
  self= [super initWithFrame: NSMakeRect(10, 10, 10, 10)];
  if (self)
  {
    mOwner = aBrowser;
    mOwner->set_data(self);
    
    mBrowser = [[[WebView alloc] initWithFrame: [self frame]] autorelease];
    [self addSubview: mBrowser];
    [mBrowser setFrameLoadDelegate: self];
    [mBrowser setUIDelegate: self];
    [mBrowser setPolicyDelegate: self];
    [mBrowser setResourceLoadDelegate: self];
    [mBrowser setShouldCloseWithWindow: YES];
  }
  return self;
}

//--------------------------------------------------------------------------------------------------

// WebView delegates.

- (void) webView: (WebView*) webView didFinishLoadForFrame: (WebFrame*) frame
{
  mOwner->document_loaded([[webView mainFrameURL] UTF8String]);
}

//--------------------------------------------------------------------------------------------------

- (void) webView: (WebView*) sender runOpenPanelForFileButtonWithResultListener: (id<WebOpenPanelResultListener>) resultListener
{
  NSOpenPanel *panel= [NSOpenPanel openPanel];
  [panel setTitle: @"Select file to upload"];
  
  if ([panel runModal] == NSFileHandlingPanelOKButton)
    [resultListener chooseFilename: panel.URL.path];
  else
    [resultListener cancel];
}

- (NSURLRequest *)webView:(WebView *)sender
                 resource:(id)identifier
          willSendRequest:(NSURLRequest *)request 
         redirectResponse:(NSURLResponse *)redirectResponse 
           fromDataSource:(WebDataSource *)dataSource
{  
  // disable caching
  NSMutableURLRequest *req = [[request mutableCopy] autorelease];
  [req setCachePolicy: NSURLRequestReloadIgnoringLocalCacheData];
  return req;
}

//--------------------------------------------------------------------------------------------------

- (void)webView:(WebView *)webView decidePolicyForNavigationAction:(NSDictionary *)actionInformation 
        request:(NSURLRequest *)request
          frame:(WebFrame *)frame
decisionListener:(id < WebPolicyDecisionListener >)listener
{
  if ([[actionInformation objectForKey: WebActionNavigationTypeKey] integerValue] == WebNavigationTypeLinkClicked)
  {
    NSURL *url = [[actionInformation objectForKey: WebActionElementKey] objectForKey: WebElementLinkURLKey];
    if (url && mOwner->on_link_clicked([[url description] UTF8String]))
    {
      [listener ignore];
      return;
    }
  }
  [listener use];
}

//--------------------------------------------------------------------------------------------------

- (NSString*) description
{
  return [NSString stringWithFormat: @"<%@ '%@'>", [self className], [self documentTitle]];
}

//--------------------------------------------------------------------------------------------------

- (mforms::Object*) mformsObject
{
  return NULL;
}

//--------------------------------------------------------------------------------------------------

- (void)setFrame:(NSRect)frame
{
  [super setFrame: frame];
  [mBrowser setFrame: frame];
}

//--------------------------------------------------------------------------------------------------

- (void)dealloc
{
  [super dealloc];
}

//--------------------------------------------------------------------------------------------------

- (void) setHTML: (NSString*) code
{
  [[mBrowser mainFrame] loadHTMLString: code baseURL: [NSURL URLWithString: @"/"]];
}

//--------------------------------------------------------------------------------------------------

- (void) navigate: (NSString*) target
{
  NSURL* url = [NSURL URLWithString: target];
  
  [[mBrowser mainFrame] loadRequest: [NSURLRequest requestWithURL: url 
                                                      cachePolicy: NSURLRequestReloadIgnoringCacheData
                                                  timeoutInterval: 60.0]];
}

//--------------------------------------------------------------------------------------------------

- (NSString*) documentTitle
{
  return [mBrowser stringByEvaluatingJavaScriptFromString: @"document.title"];
}

//--------------------------------------------------------------------------------------------------

static bool WebBrowser_create(::mforms::WebBrowser *self)
{
  [[[MFWebBrowserImpl alloc] initWithObject: self] autorelease];
  
  return true;  
}

//--------------------------------------------------------------------------------------------------

static void WebBrowser_set_html(::mforms::WebBrowser *self, const std::string& code)
{
  if (self)
  {
    MFWebBrowserImpl* browser = self->get_data();
    if (browser)
      [browser setHTML: [NSString stringWithUTF8String: code.c_str()]];
  }
}

//--------------------------------------------------------------------------------------------------

static void WebBrowser_navigate(::mforms::WebBrowser *self, const std::string& url)
{
  if (self)
  {
    MFWebBrowserImpl* browser = self->get_data();
    if (browser)
      [browser navigate: [NSString stringWithUTF8String: url.c_str()]];
  }
}

//--------------------------------------------------------------------------------------------------

static std::string WebBrowser_get_document_title(::mforms::WebBrowser *self)
{
  if (self)
  {
    MFWebBrowserImpl* browser = self->get_data();
    if (browser)
      return [[browser documentTitle] UTF8String];
  }
  return "";
}

//--------------------------------------------------------------------------------------------------

void cf_webbrowser_init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();
  
  f->_webbrowser_impl.create= &WebBrowser_create;
  f->_webbrowser_impl.set_html= &WebBrowser_set_html;
  f->_webbrowser_impl.navigate= &WebBrowser_navigate;
  f->_webbrowser_impl.get_document_title= &WebBrowser_get_document_title;
}

//--------------------------------------------------------------------------------------------------

@end


