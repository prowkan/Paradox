// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Interop;
using System.Runtime.InteropServices;
using System.Threading;
using System.Collections.ObjectModel;

namespace Editor
{
    public struct EntitiesListItem
    {
        public string EntityName;
        public IntPtr Entity;

        public EntitiesListItem(string EntityName, IntPtr Entity)
        {
            this.EntityName = EntityName;
            this.Entity = Entity;
        }

        public override string ToString()
        {
            return EntityName;
        }
    }

    /// <summary>
    /// Interaction logic for MainLevelEditorWindow.xaml
    /// </summary>
    public partial class MainLevelEditorWindow : Window
    {
        private EditorEngine editorEngine;

        public ObservableCollection<EntitiesListItem> entitiesListItems;

        public MainLevelEditorWindow()
        {
            InitializeComponent();
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            editorEngine.StopEditorEngine();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            editorEngine = new EditorEngine(this.WFHost.Child.Handle);
            editorEngine.EditorViewportWidth = (uint)this.WFHost.Child.Width;
            editorEngine.EditorViewportHeight = (uint)this.WFHost.Child.Height;
            editorEngine.StartEditorEngine();

            entitiesListItems = new ObservableCollection<EntitiesListItem>();

            LevelEnitiesList.ItemsSource = entitiesListItems;
        }

        [DllImport("user32.dll")]
        private extern static int ShowCursor(bool bShow);

        [DllImport("user32.dll")]
        private extern static bool SetCursorPos(int X, int Y);

        private void LevelEnitiesList_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ListBox listBox = (ListBox)sender;
            EntitiesListItem listBoxItem = (EntitiesListItem)listBox.Items[listBox.SelectedIndex];
            string EntityName = listBoxItem.EntityName;
            IntPtr Entity = listBoxItem.Entity;
            string EntityClassName = System.Runtime.InteropServices.Marshal.PtrToStringAnsi(EditorEngine.GetEntityClassName(Entity));
            StatusBarLabel.Content = EntityClassName;

            PropertiesStack.Children.Clear();

            uint EntityPropertiesCount = EditorEngine.GetEntityPropertiesCount(Entity);

            for (uint i = 0; i < EntityPropertiesCount; i++)
            {
                string EntityPropertyName = System.Runtime.InteropServices.Marshal.PtrToStringAnsi(EditorEngine.GetEntityPropertyName(Entity, i));
                EditorEngine.ClassPropertyType EntityPropertyType = EditorEngine.GetEntityPropertyType(Entity, i);

                Label PropertyLabel = new Label();
                PropertyLabel.Content = EntityPropertyName;
                PropertiesStack.Children.Add(PropertyLabel);

                if (EntityPropertyType == EditorEngine.ClassPropertyType.ComponentReference)
                {
                    IntPtr ComponentReference = EditorEngine.GetEntityComponentReferenceProperty(Entity, EntityPropertyName);

                    uint ComponentPropertiesCount = EditorEngine.GetComponentPropertiesCount(ComponentReference);

                    for (uint j = 0; j < ComponentPropertiesCount; j++)
                    {
                        string ComponentPropertyName = System.Runtime.InteropServices.Marshal.PtrToStringAnsi(EditorEngine.GetComponentPropertyName(ComponentReference, j));
                        EditorEngine.ClassPropertyType ComponentPropertyType = EditorEngine.GetComponentPropertyType(ComponentReference, j);

                        Label PropertyLabel1 = new Label();
                        PropertyLabel1.Content = ComponentPropertyName + ": ";
                        //PropertiesStack.Children.Add(PropertyLabel1);

                        StackPanel PropertyStackPanel = new StackPanel();
                        PropertyStackPanel.Orientation = Orientation.Horizontal;

                        PropertyStackPanel.Children.Add(PropertyLabel1);

                        if (ComponentPropertyType == EditorEngine.ClassPropertyType.Float)
                        {
                            float Value = EditorEngine.GetComponentFloatProperty(ComponentReference, ComponentPropertyName);
                            TextBox textBox = new TextBox();
                            textBox.Text = Value.ToString();
                            textBox.Width = 50.0;
                            textBox.TextChanged += (object Sender, TextChangedEventArgs Args) =>
                            {
                                string NewValueStr = ((TextBox)Sender).Text;

                                try
                                {
                                    float NewValue = System.Convert.ToSingle(NewValueStr);

                                    EditorEngine.SetComponentFloatProperty(ComponentReference, ComponentPropertyName, NewValue);
                                }
                                catch
                                {

                                }
                            };
                            PropertyStackPanel.Children.Add(textBox);
                        }
                        else if (ComponentPropertyType == EditorEngine.ClassPropertyType.Vector)
                        {
                            EditorEngine.Float3 Value = EditorEngine.GetComponentVectorProperty(ComponentReference, ComponentPropertyName);
                            Label labelX = new Label();
                            Label labelY = new Label();
                            Label labelZ = new Label();
                            labelX.Content = "X: ";
                            labelY.Content = "Y: ";
                            labelZ.Content = "Z: ";
                            TextBox textBoxX = new TextBox();
                            TextBox textBoxY = new TextBox();
                            TextBox textBoxZ = new TextBox();
                            textBoxX.Text = Value.X.ToString();
                            textBoxX.Width = 50.0;
                            textBoxY.Text = Value.Y.ToString();
                            textBoxY.Width = 50.0;
                            textBoxZ.Text = Value.Z.ToString();
                            textBoxZ.Width = 50.0;
                            textBoxX.TextChanged += (object Sender, TextChangedEventArgs Args) =>
                            {
                                string NewValueStr = ((TextBox)Sender).Text;

                                try 
                                {
                                    EditorEngine.Float3 NewValue;
                                    NewValue.X = System.Convert.ToSingle(NewValueStr);
                                    NewValue.Y = System.Convert.ToSingle(textBoxY.Text);
                                    NewValue.Z = System.Convert.ToSingle(textBoxZ.Text);

                                    EditorEngine.SetComponentVectorProperty(ComponentReference, ComponentPropertyName, NewValue);
                                }
                                catch
                                {

                                }
                            };
                            textBoxY.TextChanged += (object Sender, TextChangedEventArgs Args) =>
                            {
                                string NewValueStr = ((TextBox)Sender).Text;

                                try
                                {
                                    EditorEngine.Float3 NewValue;
                                    NewValue.X = System.Convert.ToSingle(textBoxX.Text);
                                    NewValue.Y = System.Convert.ToSingle(NewValueStr);
                                    NewValue.Z = System.Convert.ToSingle(textBoxZ.Text);

                                    EditorEngine.SetComponentVectorProperty(ComponentReference, ComponentPropertyName, NewValue);
                                }
                                catch
                                {

                                }
                            };
                            textBoxZ.TextChanged += (object Sender, TextChangedEventArgs Args) =>
                            {
                                string NewValueStr = ((TextBox)Sender).Text;

                                try
                                {
                                    EditorEngine.Float3 NewValue;
                                    NewValue.X = System.Convert.ToSingle(textBoxX.Text);
                                    NewValue.Y = System.Convert.ToSingle(textBoxY.Text);
                                    NewValue.Z = System.Convert.ToSingle(NewValueStr);

                                    EditorEngine.SetComponentVectorProperty(ComponentReference, ComponentPropertyName, NewValue);
                                }
                                catch
                                {

                                }
                            };
                            PropertyStackPanel.Children.Add(labelX);
                            PropertyStackPanel.Children.Add(textBoxX);
                            PropertyStackPanel.Children.Add(labelY);
                            PropertyStackPanel.Children.Add(textBoxY);
                            PropertyStackPanel.Children.Add(labelZ);
                            PropertyStackPanel.Children.Add(textBoxZ);
                        }
                        else if (ComponentPropertyType == EditorEngine.ClassPropertyType.Rotator)
                        {
                            EditorEngine.Rotator Value = EditorEngine.GetComponentRotatorProperty(ComponentReference, ComponentPropertyName);
                            Label labelPitch = new Label();
                            Label labelYaw = new Label();
                            Label labelRoll = new Label();
                            labelPitch.Content = "Pitch: ";
                            labelYaw.Content = "Yaw: ";
                            labelRoll.Content = "Roll: ";
                            TextBox textBoxPitch = new TextBox();
                            TextBox textBoxYaw = new TextBox();
                            TextBox textBoxRoll = new TextBox();
                            textBoxPitch.Text = Value.Pitch.ToString();
                            textBoxPitch.Width = 50.0;
                            textBoxYaw.Text = Value.Yaw.ToString();
                            textBoxYaw.Width = 50.0;
                            textBoxRoll.Text = Value.Roll.ToString();
                            textBoxRoll.Width = 50.0;
                            PropertyStackPanel.Children.Add(labelPitch);
                            PropertyStackPanel.Children.Add(textBoxPitch);
                            PropertyStackPanel.Children.Add(labelYaw);
                            PropertyStackPanel.Children.Add(textBoxYaw);
                            PropertyStackPanel.Children.Add(labelRoll);
                            PropertyStackPanel.Children.Add(textBoxRoll);
                        }
                        else if (ComponentPropertyType == EditorEngine.ClassPropertyType.Color)
                        {
                            EditorEngine.Color Value = EditorEngine.GetComponentColorProperty(ComponentReference, ComponentPropertyName);
                            Label labelR = new Label();
                            Label labelG = new Label();
                            Label labelB = new Label();
                            labelR.Content = "Red: ";
                            labelG.Content = "Green: ";
                            labelB.Content = "Blue: ";
                            TextBox textBoxR = new TextBox();
                            TextBox textBoxG = new TextBox();
                            TextBox textBoxB = new TextBox();
                            textBoxR.Text = Value.R.ToString();
                            textBoxR.Width = 50.0;
                            textBoxG.Text = Value.G.ToString();
                            textBoxG.Width = 50.0;
                            textBoxB.Text = Value.B.ToString();
                            textBoxB.Width = 50.0;
                            PropertyStackPanel.Children.Add(labelR);
                            PropertyStackPanel.Children.Add(textBoxR);
                            PropertyStackPanel.Children.Add(labelG);
                            PropertyStackPanel.Children.Add(textBoxG);
                            PropertyStackPanel.Children.Add(labelB);
                            PropertyStackPanel.Children.Add(textBoxB);
                        }
                        else if (ComponentPropertyType == EditorEngine.ClassPropertyType.ResourceReference)
                        {
                            IntPtr Resource = EditorEngine.GetComponentResourceReferenceProperty(ComponentReference, ComponentPropertyName);
                            string ResourceName = System.Runtime.InteropServices.Marshal.PtrToStringAnsi(EditorEngine.GetResourceName(Resource));
                            TextBox textBox = new TextBox();
                            textBox.Text = ResourceName;
                            textBox.Width = 300.0;
                            PropertyStackPanel.Children.Add(textBox);
                        }

                        PropertiesStack.Children.Add(PropertyStackPanel);
                    }
                }
            }
        }

        private int X, Y;
        private bool bIsMouseCaptured;

        private void LevelRenderPanel_MouseDown(object sender, System.Windows.Forms.MouseEventArgs e)
        {
            if (e.Button == System.Windows.Forms.MouseButtons.Right)
            {
                bIsMouseCaptured = true;
                ShowCursor(false);

                var Pnt = WFHost.Child.PointToScreen(new System.Drawing.Point(e.X, e.Y));

                X = Pnt.X;
                Y = Pnt.Y;                
            }
        }

        private void LevelRenderPanel_MouseUp(object sender, System.Windows.Forms.MouseEventArgs e)
        {
            if (e.Button == System.Windows.Forms.MouseButtons.Right)
            {
                bIsMouseCaptured = false;
                ShowCursor(true);
            }
        }

        private void LevelRenderPanel_MouseMove(object sender, System.Windows.Forms.MouseEventArgs e)
        {
            if (bIsMouseCaptured)
            {
                var Pnt = WFHost.Child.PointToScreen(new System.Drawing.Point(e.X, e.Y));

                EditorEngine.RotateCamera(Pnt.X - X, Pnt.Y - Y);

                SetCursorPos(X, Y);
            }
        }

        private bool bForward, bBackward, bLeft, bRight;

        private void WFHost_KeyUp(object sender, System.Windows.Input.KeyEventArgs e)
        {
            if (e.Key == Key.W) bForward = false;
            if (e.Key == Key.S) bBackward = false;
            if (e.Key == Key.A) bLeft = false;
            if (e.Key == Key.D) bRight = false;           
        }

        private void WFHost_KeyDown(object sender, System.Windows.Input.KeyEventArgs e)
        {
            if (e.Key == Key.W) bForward = true;
            if (e.Key == Key.S) bBackward = true;
            if (e.Key == Key.A) bLeft = true;
            if (e.Key == Key.D) bRight = true;

            EditorEngine.MoveCamera(bForward, bBackward, bLeft, bRight);
        }

        private void LevelRenderPanel_KeyDown(object sender, System.Windows.Forms.KeyEventArgs e)
        {
            
        }

        private void LevelRenderPanel_KeyUp(object sender, System.Windows.Forms.KeyEventArgs e)
        {
            
        }
    }
}
